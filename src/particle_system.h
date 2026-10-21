#pragma once

#include <framework/opengl_includes.h>
#include <framework/shader.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <array>
#include <cstddef>
#include <random>
#include <vector>

class ParticleSystem {
public:
    void initialize(const glm::vec3& emitterPosition);
    void update(float deltaTime);
    void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int framebufferHeight);

private:
    struct Particle {
        glm::vec3 position { 0.0f };
        glm::vec3 velocity { 0.0f };
        float age { 0.0f };
        float lifetime { 0.0f };
        float size { 0.0f }; // diameter metres
    };

    // we put on the gpu only what we need to draw the particle
    struct ParticleVertex {
        glm::vec3 position;
        float size;
        float opacity;
    };

    static const std::size_t MAX_PARTICLES = 128;
    static const float EMISSION_RATE = 40.0f; // particles per second

    void spawnParticle();
    float randomFloat(float minimum, float maximum);

    std::array<Particle, MAX_PARTICLES> m_particles {};
    std::vector<ParticleVertex> m_vertices;
    glm::vec3 m_emitterPosition { 0.0f };
    float m_emissionAccumulator { 0.0f };
    std::mt19937 m_randomGenerator { 2137 }; // repeatable initial sequence

    Shader m_shader;
    GLuint m_vao { 0 };
    GLuint m_vbo { 0 };
};
