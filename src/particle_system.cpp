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
   
    // a particle is basically a small 2d square in the world. the position of each ParticleVertex is the center of that square
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(MAX_PARTICLES * sizeof(ParticleVertex)), nullptr, GL_STREAM_DRAW);

    // 3 floats for position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
        reinterpret_cast<const void*>(offsetof(ParticleVertex, position)));
    // 1 float for size
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
        reinterpret_cast<const void*>(offsetof(ParticleVertex, size)));
    // 1 float for opacity
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
        reinterpret_cast<const void*>(offsetof(ParticleVertex, opacity)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// update the age and position for each particle and spawn new particles
void ParticleSystem::update(float deltaTime)
{
    for (Particle& particle : m_particles) {
        // if the particle age is more than equal to the lifetime than that particle is dead
        if (particle.age >= particle.lifetime)
            continue;

        particle.age += deltaTime;
        particle.position += particle.velocity * deltaTime;
    }

    // we want EMISSION_RAFE particles per second so we store how many particles we need to spawn since
    // the last update call in m_emissionAccumulator
    m_emissionAccumulator += EMISSION_RATE * deltaTime;
    // spawn the particles
    while (m_emissionAccumulator >= 1.0f) {
        m_emissionAccumulator -= 1.0f;
        spawnParticle();
    }
}

// the emitter is a horizontal disk so we need a random position on that disk to put a new particle in
// also make the lifetime and size and direction slightly random for the look
void ParticleSystem::spawnParticle()
{
    for (Particle& particle : m_particles) {
        // we are looking for a dead particle in the array to replace with our new one
        if (particle.age < particle.lifetime)
            continue;

        // for a random position on the disk we need a random distance from the center (radius) and random angle
        const float angle = randomFloat(0.0f, glm::two_pi<float>());
        // use sqrt because otherwise there will be more particles near the center
        const float radius = 0.18f * std::sqrt(randomFloat(0.0f, 1.0f));

        // upwards and slightly to the sides
        particle.velocity = glm::vec3(
            randomFloat(-0.12f, 0.12f), randomFloat(0.44f, 0.88f), randomFloat(-0.12f, 0.12f));
        particle.age = 0.0f;
        particle.lifetime = randomFloat(1.5f, 2.5f);
        particle.size = randomFloat(0.044f, 0.09f);
        particle.position = m_emitterPosition + glm::vec3(radius * std::cos(angle), 0.0f, radius * std::sin(angle));
        return;
    }
}

float ParticleSystem::randomFloat(float minimum, float maximum)
{
    std::uniform_real_distribution<float> distribution(minimum, maximum);
    return distribution(m_randomGenerator);
}

// for each alive particle we calculate the opacity and put it into m_vertices. then we draw the particles in m_vertices
void ParticleSystem::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int framebufferHeight)
{
    m_vertices.clear();

    for (const Particle& particle : m_particles) {
        // if dead skip
        if (particle.age >= particle.lifetime)
            continue;

        // we want the particle to start invisible, fade in quickly, and then fade out during the rest of its life
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

    // we want depth test enabled so things can cover the particles but we dont want to write depth
    // because the particles shouldnt cover things for rendering. we just add them on top and blend
    glDepthMask(GL_FALSE);
    // make it so that for particles the gl_PointSize from the vertex shader is used for the size of the particle
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    // gl points
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_vertices.size()));

    // return to the settings used by normal scene rendering
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}
