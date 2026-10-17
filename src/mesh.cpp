#include "mesh.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <iostream>
#include <utility>
#include <vector>

GPUMaterial::GPUMaterial(const Material& material) :
    kd(material.kd),
    ks(material.ks),
    shininess(material.shininess),
    transparency(material.transparency)
{}

GPUMesh::GPUMesh(const Mesh& cpuMesh)
    : m_materialName(cpuMesh.material.name)
{
    if (cpuMesh.material.kdTexture)
        m_baseColorTexture.emplace(*cpuMesh.material.kdTexture, TextureColorSpace::Srgb);

    // Create uniform buffer to store mesh material (https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL)
    GPUMaterial gpuMaterial(cpuMesh.material);
    glGenBuffers(1, &m_uboMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, m_uboMaterial);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUMaterial), &gpuMaterial, GL_STATIC_DRAW);

    // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create vertex buffer object (VBO)
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.vertices.size() * sizeof(decltype(cpuMesh.vertices)::value_type)), cpuMesh.vertices.data(), GL_STATIC_DRAW);

    // Create index buffer object (IBO)
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.triangles.size() * sizeof(decltype(cpuMesh.triangles)::value_type)), cpuMesh.triangles.data(), GL_STATIC_DRAW);

    // Tell OpenGL that we will be using vertex attributes 0, 1 and 2.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    // We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    // Reuse all attributes for each instance
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);

    // Each triangle has 3 vertices.
    m_numIndices = static_cast<GLsizei>(3 * cpuMesh.triangles.size());
}

GPUMesh::GPUMesh(GPUMesh&& other) noexcept
{
    moveInto(std::move(other));
}

GPUMesh::~GPUMesh()
{
    freeGpuMemory();
}

GPUMesh& GPUMesh::operator=(GPUMesh&& other) noexcept
{
    if (this != &other)
        moveInto(std::move(other));
    return *this;
}

std::vector<GPUMesh> GPUMesh::loadMeshGPU(std::filesystem::path filePath, bool normalize) {
    if (!std::filesystem::exists(filePath))
        throw MeshLoadingException(fmt::format("File {} does not exist", filePath.string().c_str()));

    // Generate GPU-side meshes for all sub-meshes
    std::vector<Mesh> subMeshes = loadMesh(filePath, { .normalizeVertexPositions = normalize });
    std::vector<GPUMesh> gpuMeshes;
    gpuMeshes.reserve(subMeshes.size());
    size_t triangleCount = 0;
    std::cout << "Loading model: " << filePath.string() << '\n';
    for (const Mesh& mesh : subMeshes) {
        gpuMeshes.emplace_back(mesh);
        triangleCount += mesh.triangles.size();
        std::cout << "  Material '" << gpuMeshes.back().materialName() << "': "
                  << mesh.triangles.size() << " triangles, "
                  << (gpuMeshes.back().hasBaseColorTexture() ? "base-color texture" : "constant base color")
                  << '\n';
    }
    std::cout << "Loaded " << gpuMeshes.size() << " meshes, " << triangleCount << " triangles\n";
    
    return gpuMeshes;
}

bool GPUMesh::hasBaseColorTexture() const
{
    return m_baseColorTexture.has_value();
}

const std::string& GPUMesh::materialName() const
{
    return m_materialName;
}

void GPUMesh::draw(const Shader& drawingShader) const
{
    // Bind material data uniform (we assume that the uniform buffer objects is always called 'Material')
    // Yes, we could define the binding inside the shader itself, but that would break on OpenGL versions below 4.2
    drawingShader.bindUniformBlock("Material", 0, m_uboMaterial);

    glUniform1i(drawingShader.getUniformLocation("hasBaseColorTexture"), hasBaseColorTexture());
    glUniform1i(drawingShader.getUniformLocation("baseColorMap"), 0);
    if (m_baseColorTexture)
        m_baseColorTexture->bind(GL_TEXTURE0);
    
    // Draw the mesh's triangles
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
}

void GPUMesh::moveInto(GPUMesh&& other) noexcept
{
    freeGpuMemory();
    m_numIndices = other.m_numIndices;
    m_materialName = std::move(other.m_materialName);
    m_baseColorTexture = std::move(other.m_baseColorTexture);
    m_ibo = other.m_ibo;
    m_vbo = other.m_vbo;
    m_vao = other.m_vao;
    m_uboMaterial = other.m_uboMaterial;

    other.m_numIndices = 0;
    other.m_baseColorTexture.reset();
    other.m_ibo = INVALID;
    other.m_vbo = INVALID;
    other.m_vao = INVALID;
    other.m_uboMaterial = INVALID;
}

void GPUMesh::freeGpuMemory()
{
    if (m_vao != INVALID)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != INVALID)
        glDeleteBuffers(1, &m_vbo);
    if (m_ibo != INVALID)
        glDeleteBuffers(1, &m_ibo);
    if (m_uboMaterial != INVALID)
        glDeleteBuffers(1, &m_uboMaterial);
    m_baseColorTexture.reset();
    m_vao = m_vbo = m_ibo = m_uboMaterial = INVALID;
    m_numIndices = 0;
}
