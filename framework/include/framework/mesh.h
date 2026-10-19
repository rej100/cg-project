#pragma once
#include "image.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord; // Texture coordinate
	// xyz: object-space tangent direction (along increasing normal map texture U), w: bitangent orientation sign (1 or -1).
	glm::vec4 tangentAndBitangentSign { 0.0f, 0.0f, 0.0f, 1.0f };

	[[nodiscard]] constexpr bool operator==(const Vertex&) const noexcept = default;
};

struct Material {
	std::string name { "default" };
	glm::vec3 kd; // linear rgb
	glm::vec3 ks{ 0.0f };
	float shininess{ 1.0f };
	float transparency{ 1.0f };
	float metallic{ 0.0f };
	float roughness{ 0.0f };
	glm::vec3 emission{ 0.0f }; // linear rgb

	// Optional texture that replaces kd; use as follows:
	// 
	// if (material.kdTexture) {
	//   material.kdTexture->getTexel(...);
	// }
	std::shared_ptr<Image> kdTexture;
	std::shared_ptr<Image> roughnessTexture; // optional texture that replaces roughness.
	// normal map
	std::shared_ptr<Image> normalTexture;
	float normalMapStrength{ 1.0f };
};

struct Mesh {
	// Vertices contain the vertex positions and normals of the mesh.
	std::vector<Vertex> vertices;
	// A triangle contains a triplet of values corresponding to the indices of the 3 vertices in the vertices array.
	std::vector<glm::uvec3> triangles;

	Material material;
};

struct LoadMeshSettings {
	bool normalizeVertexPositions { false };
	bool cacheVertices { true };
};

[[nodiscard]] std::vector<Mesh> loadMesh(const std::filesystem::path& file, const LoadMeshSettings& settings = {});
[[nodiscard]] Mesh mergeMeshes(std::span<const Mesh> meshes);
void meshFlipX(Mesh& mesh);
void meshFlipY(Mesh& mesh);
void meshFlipZ(Mesh& mesh);
