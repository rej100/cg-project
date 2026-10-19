#pragma once

#include <filesystem>
#include <framework/opengl_includes.h>
#include <framework/shader.h>
#include <glm/mat4x4.hpp>

// One distant environment is shared by the sky background and every object's
// reflections. Its cubemap is indexed by world-space direction, not position.
class Environment {
public:
    // Call once after creating the OpenGL context. The cubemap and cube buffers
    // stay allocated until that context is destroyed at application shutdown.
    void initialize(const std::filesystem::path& panoramaPath);

    void bind(GLenum textureSlot) const;
    void drawBackground(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
    float maxReflectionLod() const;

private:
    static constexpr int FACE_SIZE = 1024;
    Shader m_skyboxShader;
    GLuint m_cubemap { 0 };
    GLuint m_cubeVao { 0 };
    GLuint m_cubeVbo { 0 };
    float m_maxReflectionLod { 0.0f };
};
