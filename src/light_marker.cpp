#include "light_marker.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()
#include <cstdint>

namespace {
void addTriangle(Mesh& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
    const auto firstVertex = static_cast<uint32_t>(mesh.vertices.size());
    const glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
    mesh.vertices.push_back({ a, normal, glm::vec2(0.0f) });
    mesh.vertices.push_back({ b, normal, glm::vec2(0.0f) });
    mesh.vertices.push_back({ c, normal, glm::vec2(0.0f) });
    mesh.triangles.emplace_back(firstVertex, firstVertex + 1, firstVertex + 2);
}

void addQuad(Mesh& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d)
{
    addTriangle(mesh, a, b, c);
    addTriangle(mesh, a, c, d);
}

void addBox(Mesh& mesh, glm::vec3 min, glm::vec3 max)
{
    const glm::vec3 a(min.x, min.y, min.z);
    const glm::vec3 b(max.x, min.y, min.z);
    const glm::vec3 c(max.x, max.y, min.z);
    const glm::vec3 d(min.x, max.y, min.z);
    const glm::vec3 e(min.x, min.y, max.z);
    const glm::vec3 f(max.x, min.y, max.z);
    const glm::vec3 g(max.x, max.y, max.z);
    const glm::vec3 h(min.x, max.y, max.z);

    addQuad(mesh, e, f, g, h);
    addQuad(mesh, b, a, d, c);
    addQuad(mesh, a, e, h, d);
    addQuad(mesh, f, b, c, g);
    addQuad(mesh, h, g, c, d);
    addQuad(mesh, a, b, f, e);
}
}

Mesh makeSpotlightMarkerMesh()
{
    Mesh mesh;
    mesh.material.name = "spotlight_marker";
    mesh.material.kd = glm::vec3(1.0f, 0.24f, 0.015f);

    // A box at the light origin and an arrow pointing along local -Z.
    addBox(mesh, glm::vec3(-0.15f), glm::vec3(0.15f));
    addBox(mesh, glm::vec3(-0.035f, -0.035f, -0.70f), glm::vec3(0.035f, 0.035f, -0.15f));

    const glm::vec3 a(-0.18f, -0.18f, -0.70f);
    const glm::vec3 b(0.18f, -0.18f, -0.70f);
    const glm::vec3 c(0.18f, 0.18f, -0.70f);
    const glm::vec3 d(-0.18f, 0.18f, -0.70f);
    const glm::vec3 tip(0.0f, 0.0f, -1.10f);
    addQuad(mesh, a, b, c, d);
    addTriangle(mesh, a, tip, b);
    addTriangle(mesh, b, tip, c);
    addTriangle(mesh, c, tip, d);
    addTriangle(mesh, d, tip, a);

    return mesh;
}
