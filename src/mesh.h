#pragma once

#include "texture.h"
#include <framework/disable_all_warnings.h>
#include <framework/mesh.h>
#include <framework/shader.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()

#include <exception>
#include <filesystem>
#include <framework/opengl_includes.h>
#include <optional>
#include <string>

struct MeshLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Alignment directives are to comply with std140 alignment requirements (https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Memory_layout)
struct GPUMaterial {
    GPUMaterial(const Material& material);

    alignas(16) glm::vec3 kd { 1.0f }; // linear rgb from Material::kd.
    alignas(16) glm::vec3 ks { 0.0f };
    float shininess { 1.0f };
    float transparency { 1.0f };
};

class GPUMesh {
public:
    GPUMesh(const Mesh& cpuMesh);
    // Cannot copy a GPU mesh because it would require reference counting of GPU resources.
    GPUMesh(const GPUMesh&) = delete;
    GPUMesh(GPUMesh&&) noexcept;
    ~GPUMesh();

    // Generate a number of GPU meshes from a particular model file.
    // Multiple meshes may be generated if there are multiple sub-meshes in the file
    static std::vector<GPUMesh> loadMeshGPU(std::filesystem::path filePath, bool normalize = false);

    // Cannot copy a GPU mesh because it would require reference counting of GPU resources.
    GPUMesh& operator=(const GPUMesh&) = delete;
    GPUMesh& operator=(GPUMesh&&) noexcept;

    bool hasBaseColorTexture() const;
    const std::string& materialName() const;

    // Bind this mesh's material and texture, then draw its indexed geometry.
    void draw(const Shader& drawingShader) const;

private:
    void moveInto(GPUMesh&&) noexcept;
    void freeGpuMemory();

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;

    GLsizei m_numIndices { 0 };
    std::string m_materialName;
    std::optional<Texture> m_baseColorTexture;
    GLuint m_ibo { INVALID };
    GLuint m_vbo { INVALID };
    GLuint m_vao { INVALID };
    GLuint m_uboMaterial { INVALID };
};
