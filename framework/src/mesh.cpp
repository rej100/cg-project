#include "mesh.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cassert>
#include <cmath>
#include <exception>
#include <iostream>
#include <numeric>
#include <span>
#include <stack>
#include <string>
#include <tuple>
#include <map>

static void centerAndScaleToUnitMesh(std::span<Mesh> meshes);
static void generateTangents(Mesh& mesh);

static glm::vec3 construct_vec3(const float* pFloats)
{
    return glm::vec3(pFloats[0], pFloats[1], pFloats[2]);
}

// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
template <class T>
static void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct VertexHash {
    size_t operator()(const Vertex& v) const
    {
        size_t seed = 0;
        hash_combine(seed, v.position.x);
        hash_combine(seed, v.position.y);
        hash_combine(seed, v.position.z);
        hash_combine(seed, v.normal.x);
        hash_combine(seed, v.normal.y);
        hash_combine(seed, v.normal.z);
        hash_combine(seed, v.texCoord.s);
        hash_combine(seed, v.texCoord.t);
        return seed;
    }
};

std::vector<Mesh> loadMesh(const std::filesystem::path& file, const LoadMeshSettings& settings)
{
    if (!std::filesystem::exists(file)) {
        std::cerr << "File " << file << " does not exist." << std::endl;
        throw std::exception();
    }

    const auto baseDir = file.parent_path();

    tinyobj::attrib_t inAttrib;
    std::vector<tinyobj::shape_t> inShapes;
    std::vector<tinyobj::material_t> inMaterials;

    std::string warn, error;
    bool ret = tinyobj::LoadObj(&inAttrib, &inShapes, &inMaterials, &warn, &error, file.string().c_str(), baseDir.string().c_str());
    if (!ret) {
        std::cerr << "Failed to load mesh " << file << std::endl;
        throw std::exception();
    }

    std::vector<Mesh> out;
    for (const auto& shape : inShapes) {
        assert(shape.mesh.indices.size() % 3 == 0);

        const size_t numTriangles = shape.mesh.indices.size() / 3;
        size_t startTriangle = 0;
        while (startTriangle < numTriangles) {
            // Each output mesh contains one consecutive range of triangles with the same material.
            const auto materialID = shape.mesh.material_ids[startTriangle];
            size_t endTriangle = startTriangle + 1;
            while (endTriangle < numTriangles && shape.mesh.material_ids[endTriangle] == materialID)
                ++endTriangle;

            Mesh mesh;
            using CacheKey = std::tuple<uint32_t, uint32_t, uint32_t>;
            std::map<CacheKey, uint32_t> vertexCache; // Map the index of a vertex as loaded by tinyobjloader to its index in the generated mesh
            for (size_t i = startTriangle * 3; i != endTriangle * 3; i += 3) {
                const glm::vec3 v0 = construct_vec3(&inAttrib.vertices[3 * shape.mesh.indices[i + 0].vertex_index]);
                const glm::vec3 v1 = construct_vec3(&inAttrib.vertices[3 * shape.mesh.indices[i + 1].vertex_index]);
                const glm::vec3 v2 = construct_vec3(&inAttrib.vertices[3 * shape.mesh.indices[i + 2].vertex_index]);
                const auto geometricNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

                // Load the triangle indices and lazily create the vertices.
                glm::uvec3 triangle;
                for (unsigned j = 0; j < 3; j++) {
                    const auto& tinyObjIndex = shape.mesh.indices[i + j];
                    Vertex vertex {
                        .position = construct_vec3(&inAttrib.vertices[3 * tinyObjIndex.vertex_index]),
                        .normal = glm::vec3(0),
                        .texCoord = glm::vec2(0)
                    };
                    if (tinyObjIndex.normal_index != -1 && !inAttrib.normals.empty())
                        vertex.normal = glm::vec3(inAttrib.normals[3 * tinyObjIndex.normal_index + 0], inAttrib.normals[3 * tinyObjIndex.normal_index + 1], inAttrib.normals[3 * tinyObjIndex.normal_index + 2]);
                    else
                        vertex.normal = geometricNormal;
                    if (tinyObjIndex.texcoord_index != -1 && !inAttrib.texcoords.empty())
                        vertex.texCoord = glm::vec2(inAttrib.texcoords[2 * tinyObjIndex.texcoord_index + 0], inAttrib.texcoords[2 * tinyObjIndex.texcoord_index + 1]);

                    const CacheKey cacheKey { tinyObjIndex.vertex_index, tinyObjIndex.normal_index, tinyObjIndex.texcoord_index };
                    if (auto iter = vertexCache.find(cacheKey); settings.cacheVertices && iter != std::end(vertexCache)) {
                        // Already visited this vertex? Reuse it!
                        triangle[j] = iter->second;
                    } else {
                        // New vertex? Create it and store it in the vertex cache.
                        vertexCache[cacheKey] = triangle[j] = (unsigned)mesh.vertices.size();
                        mesh.vertices.push_back(vertex);
                    }
                }
                mesh.triangles.push_back(triangle);
            }

            if (materialID == -1) {
                mesh.material.kd = glm::vec3(1.0f);
                mesh.material.ks = glm::vec3(0.0f);
                mesh.material.shininess = 1.0f;
            } else {
                const auto& objMaterial = inMaterials[materialID];
                mesh.material.name = objMaterial.name;
                // linear rgb kd from assets
                mesh.material.kd = construct_vec3(objMaterial.diffuse);
                if (!objMaterial.diffuse_texname.empty()) {
                    mesh.material.kdTexture = std::make_shared<Image>(baseDir / objMaterial.diffuse_texname);
                }
                mesh.material.ks = construct_vec3(objMaterial.specular);
                mesh.material.shininess = objMaterial.shininess;
                mesh.material.transparency = objMaterial.dissolve;
                mesh.material.metallic = objMaterial.metallic;
                mesh.material.roughness = objMaterial.roughness;
                if (!objMaterial.roughness_texname.empty()) {
                    mesh.material.roughnessTexture = std::make_shared<Image>(baseDir / objMaterial.roughness_texname);
                }
                // Blender exports tangent-space normal maps as map_Bump with a -bm strength.
                const std::string& normalMapName = !objMaterial.normal_texname.empty() ? objMaterial.normal_texname : objMaterial.bump_texname;
                if (!normalMapName.empty()) {
                    mesh.material.normalTexture = std::make_shared<Image>(baseDir / normalMapName);
                    mesh.material.normalMapStrength = static_cast<float>(objMaterial.bump_texopt.bump_multiplier);
                }
                mesh.material.emission = construct_vec3(objMaterial.emission);
            }

            out.push_back(std::move(mesh));

            startTriangle = endTriangle;
        }
    }

    for (Mesh& mesh : out)
        generateTangents(mesh);

    if (settings.normalizeVertexPositions)
        centerAndScaleToUnitMesh(out);

    return out;
}

// calculate the object space tangent and bitangent sign for every vertex
//
// for each triangle calculate the tangent and bitangent then add that tangent to
// the sum for each of the 3 vertices of the triangle. also for each triangle, and 1 to the sign vote sum of
// each of its vertices if the sign of the bitangent of the triangle is the same when reconstructed from the
// tangent and the normal

// for each vertex make the combined tangent perpendicualr its vertex normal and then normalize it.
// also for each vertex the btangent sign is positive if the sum is positive and negative if the sum is negative
//
// Store the tangent in xyz and the bitangent sign in w of
// tangentAndBitangentSign.
static void generateTangents(Mesh& mesh)
{
    const size_t vertexCount = mesh.vertices.size();
    std::vector<glm::vec3> tangentSumsPerVertex(vertexCount, glm::vec3(0.0f));
    std::vector<float> bitangentSignVotes(vertexCount, 0.0f);

    for (const glm::uvec3& triangle : mesh.triangles) {
        const Vertex& v0 = mesh.vertices[triangle[0]];
        const Vertex& v1 = mesh.vertices[triangle[1]];
        const Vertex& v2 = mesh.vertices[triangle[2]];

        const glm::vec3 edge1 = v1.position - v0.position;
        const glm::vec3 edge2 = v2.position - v0.position;
        const glm::vec2 duv1 = v1.texCoord - v0.texCoord;
        const glm::vec2 duv2 = v2.texCoord - v0.texCoord;

        // [edge1 edge2] = [T B] * [[duv1.x duv1.y] [duv2.x duv2.y]] solve for T and B
        const float determinant = duv1.x * duv2.y - duv1.y * duv2.x;
        if (std::abs(determinant) < 0.000001f)
            continue;

        const float inverseDeterminant = 1.0f / determinant;
        const glm::vec3 triangleTangent = inverseDeterminant * (duv2.y * edge1 - duv1.y * edge2);
        const glm::vec3 triangleBitangent = inverseDeterminant * (-duv2.x * edge1 + duv1.x * edge2);

        for (unsigned j = 0; j < 3; ++j) {
            const unsigned vertexIndex = triangle[j];
            tangentSumsPerVertex[vertexIndex] += triangleTangent;

            const glm::vec3 vertexNormal = mesh.vertices[vertexIndex].normal;
            const glm::vec3 bitangentFromCross = glm::cross(vertexNormal, triangleTangent);
            const float bitangentAlignment = glm::dot(bitangentFromCross, triangleBitangent);

            if (bitangentAlignment >= 0.0f) {
                bitangentSignVotes[vertexIndex] += 1.0f;
            } else {
                bitangentSignVotes[vertexIndex] -= 1.0f;
            }
        }
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        const glm::vec3 normal = glm::normalize(mesh.vertices[i].normal);
        glm::vec3 tangent = tangentSumsPerVertex[i];

        const float tangentLengthSquared = glm::dot(tangent, tangent);

        // if we get 0 vector we there will be a problem if we try to normalize so we
        // need to choose a different one (that is perpendicular to the vertex normal)
        if (tangentLengthSquared < 0.000001f) {
            // use the x axis 
            glm::vec3 referenceAxis(1.0f, 0.0f, 0.0f);

            // if the normal is close to paralle to the x axis choose the y axis instead
            if (std::abs(normal.x) > 0.9f) {
                referenceAxis = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // make the vector we chose perpendicular to the vertex normal
            tangent = glm::cross(normal, referenceAxis);
        }

        // make the tangent we got from the sum of the triangle tangents perpendicular to the normal of the vertex
        const float componentAlongNormal = glm::dot(normal, tangent);
        tangent -= normal * componentAlongNormal;

        // Store a unit direction.
        tangent = glm::normalize(tangent);

        float bitangentSign = 1.0f;
        if (bitangentSignVotes[i] < 0.0f) {
            bitangentSign = -1.0f;
        }
        mesh.vertices[i].tangentAndBitangentSign = glm::vec4(tangent, bitangentSign);
    }
}

static void centerAndScaleToUnitMesh(std::span<Mesh> meshes)
{
    std::vector<glm::vec3> positions;
    for (const auto& mesh : meshes)
        std::transform(std::begin(mesh.vertices), std::end(mesh.vertices),
            std::back_inserter(positions),
            [](const Vertex& v) { return v.position; });
    const glm::vec3 center = std::accumulate(std::begin(positions), std::end(positions), glm::vec3(0.0f)) / static_cast<float>(positions.size());
    float maxD = 0.0f;
    for (const glm::vec3& p : positions)
        maxD = std::max(glm::length(p - center), maxD);
    /*// REQUIRES A MODERN COMPILER
      const float maxD = std::transform_reduce(
              std::begin(vertices), std::end(vertices),
              0.0f,
              [](float lhs, float rhs) { return std::max(lhs, rhs); },
              [=](const Vertex& v) { return glm::length(v.pos - center); });*/

    for (auto& mesh : meshes) {
        std::transform(std::begin(mesh.vertices), std::end(mesh.vertices),
            std::begin(mesh.vertices), [=](Vertex v) {
                v.position = (v.position - center) / maxD;
                return v;
            });
    }
}

Mesh mergeMeshes(std::span<const Mesh> meshes)
{
    Mesh out;
    out.material = meshes[0].material;
    for (const auto& mesh : meshes) {
        const auto vertexOffset = out.vertices.size();
        out.vertices.resize(out.vertices.size() + mesh.vertices.size());
        std::copy(std::begin(mesh.vertices), std::end(mesh.vertices), std::begin(out.vertices) + vertexOffset);

        for (const auto& tri : mesh.triangles) {
            out.triangles.push_back(tri + (unsigned)vertexOffset);
        }
    }
    return out;
}

void meshFlipX(Mesh& mesh)
{
    for (auto& v : mesh.vertices) {
        v.position.x = -v.position.x;
        v.normal.x = -v.normal.x;
    }
}

void meshFlipY(Mesh& mesh)
{
    for (auto& v : mesh.vertices) {
        v.position.y = -v.position.y;
        v.normal.y = -v.normal.y;
    }
}

void meshFlipZ(Mesh& mesh)
{
    for (auto& v : mesh.vertices) {
        v.position.z = -v.position.z;
        v.normal.z = -v.normal.z;
    }
}
