#pragma once

#include <filesystem>
#include <framework/opengl_includes.h>
#include <framework/shader.h>
#include <glm/mat4x4.hpp>

// one environemnt for the background and reflections. we treat it as being infinitely distant.
// the cubemap is sampled with world space direction
class Environment {
public:
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
