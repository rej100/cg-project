#pragma once

#include <framework/opengl_includes.h>
#include <framework/shader.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <array>
#include <cstddef>
#include <vector>

// closed loop of cubic bezier segments that the drone flies along
// the global parameter u goes from 0 to segmentCount() and wraps back to 0 so drone continues flying
class FlightPath {
public:
    void initialize();
    void draw(const glm::mat4& viewProjectionMatrix);

    glm::vec3 position(float u) const;
    glm::vec3 tangent(float u) const; // derivative of parameter u, points forward as camera pov

    float length() const;
    float parameterAtDistance(float distance) const;
    std::size_t segmentCount() const; // 5 total

private:
    // a joint has its position and the handle that comes after it 
    // the handle before it reuses the one after (2 * joint - after)
    struct Joint {
        glm::vec3 position;
        glm::vec3 handleAfter;
    };

    static constexpr std::size_t SAMPLES_PER_SEGMENT = 200; // so 1000 points total

    std::array<glm::vec3, 4> segmentPoints(std::size_t segment) const;
    void segmentAndLocal(float u, std::size_t& segment, float& t) const;
    void buildArcLengthTable();

    std::vector<Joint> m_joints;
    // pairs of (distance along the path, u) for constant speed
    std::vector<float> m_tableDistances;
    std::vector<float> m_tableParameters;

    Shader m_shader;
    GLuint m_vao { 0 };
    GLuint m_vbo { 0 };
    GLsizei m_vertexCount { 0 };
};
