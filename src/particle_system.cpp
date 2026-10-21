#include "particle_system.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>

void ParticleSystem::initialize(const glm::vec3& emitterPosition)
{
    m_emitterPosition = emitterPosition;
    m_vertices.reserve(MAX_PARTICLES);

    // build the shader for drawing the particles
    ShaderBuilder builder;
    builder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/particle_vert.glsl");
    builder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/particle_frag.glsl");
    m_shader = builder.build();

    // create the vao and vbo for the particles
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(MAX_PARTICLES * sizeof(ParticleVertex)), nullptr, GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
        reinterpret_cast<const void*>(offsetof(ParticleVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
        reinterpret_cast<const void*>(offsetof(ParticleVertex, size)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
        reinterpret_cast<const void*>(offsetof(ParticleVertex, opacity)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::update(float deltaTime)
{
    for (Particle& particle : m_particles) {
        if (particle.age >= particle.lifetime)
            continue;

        particle.age += deltaTime;
        particle.position += particle.velocity * deltaTime;
    }

    // keep the fractional part, so emission is measured per second rather than per frame
    m_emissionAccumulator += EMISSION_RATE * deltaTime;
    while (m_emissionAccumulator >= 1.0f) {
        m_emissionAccumulator -= 1.0f;
        spawnParticle();
    }
}

void ParticleSystem::spawnParticle()
{
    for (Particle& particle : m_particles) {
        if (particle.age < particle.lifetime)
            continue;

        const float angle = randomFloat(0.0f, glm::two_pi<float>());
        // sqrt distributes births uniformly over the area of the emitter disk
        const float radius = 0.18f * std::sqrt(randomFloat(0.0f, 1.0f));
        particle.velocity = glm::vec3(
            randomFloat(-0.12f, 0.12f), randomFloat(0.44f, 0.88f), randomFloat(-0.12f, 0.12f));
        particle.age = 0.0f;
        particle.lifetime = randomFloat(1.5f, 2.5f);
        particle.size = randomFloat(0.044f, 0.09f);
        particle.position = m_emitterPosition
            + glm::vec3(radius * std::cos(angle), 0.0f, radius * std::sin(angle));
        return;
    }
}

float ParticleSystem::randomFloat(float minimum, float maximum)
{
    std::uniform_real_distribution<float> distribution(minimum, maximum);
    return distribution(m_randomGenerator);
}

void ParticleSystem::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int framebufferHeight)
{
    m_vertices.clear();
    for (const Particle& particle : m_particles) {
        if (particle.age >= particle.lifetime)
            continue;

        // fade in briefly at birth, then fade away over the particle's lifetime
        const float fadeIn = std::min(particle.age / 0.12f, 1.0f);
        const float opacity = fadeIn * (1.0f - particle.age / particle.lifetime);
        m_vertices.push_back({ particle.position, particle.size, opacity });
    }
    if (m_vertices.empty())
        return;

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(ParticleVertex)), m_vertices.data());

    m_shader.bind();
    glUniformMatrix4fv(m_shader.getUniformLocation("viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(m_shader.getUniformLocation("projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniform1f(m_shader.getUniformLocation("framebufferHeight"), static_cast<float>(framebufferHeight));

    // keep the opaque scene's depth test, but do not let a soft dot write depth
    glDepthMask(GL_FALSE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_vertices.size()));

    // return to the settings used by normal scene rendering
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}
