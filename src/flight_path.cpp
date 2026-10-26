#include "flight_path.h"
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

void FlightPath::initialize()
{
    // joints store their position and the handle after them (for logic see flight_path.h)
    const auto joint = [](glm::vec3 position, glm::vec3 offset) { return Joint { position, position + offset }; };
    m_joints = {
        joint({ 0.0f, 4.5f, 5.0f }, { 2.2f, -0.6f, 0.0f }), // 1: in front of reactor
        joint({ 2.4f, 2.4f, 2.9f }, { 1.1f, 0.0f, 0.6f }), // 2: start of the loop around the environment mapping sphere
        joint({ 4.8f, 2.0f, 1.5f }, { 0.0f, 0.0f, -1.3f }), // 3: far side of the sphere
        joint({ 2.6f, 2.4f, 0.0f }, { -1.2f, 0.3f, -0.2f }), // 4: end of the loop around the sphere
        joint({ 1.0f, 5.2f, -3.2f }, { -1.6f, 0.0f, 0.3f }), // 5: back of the reactor
    };

    buildArcLengthTable();

    // loop over 5 segments and populate each with 200 points connected back to 0th segment
    // with simple path shader
    std::vector<glm::vec3> vertices;
    const std::size_t sampleCount = segmentCount() * SAMPLES_PER_SEGMENT;
    for (std::size_t i = 0; i < sampleCount; ++i)
        vertices.push_back(position(static_cast<float>(i) / SAMPLES_PER_SEGMENT));
    m_vertexCount = static_cast<GLsizei>(vertices.size());

    ShaderBuilder builder;
    builder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/path_vert.glsl");
    builder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/path_frag.glsl");
    m_shader = builder.build();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void FlightPath::draw(const glm::mat4& viewProjectionMatrix)
{
    m_shader.bind();
    glUniformMatrix4fv(m_shader.getUniformLocation("viewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINE_LOOP, 0, m_vertexCount);
    glBindVertexArray(0);
}

// B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3
glm::vec3 FlightPath::position(float u) const
{
    std::size_t segment;
    float t;
    segmentAndLocal(u, segment, t);
    const std::array<glm::vec3, 4> p = segmentPoints(segment);

    const float s = 1.0f - t;
    return s * s * s * p[0] + 3.0f * s * s * t * p[1] + 3.0f * s * t * t * p[2] + t * t * t * p[3];
}

// B'(t) = 3(1-t)^2 (P1-P0) + 6(1-t) t (P2-P1) + 3t^2 (P3-P2)
// derivative, where drone faces
glm::vec3 FlightPath::tangent(float u) const
{
    std::size_t segment;
    float t;
    segmentAndLocal(u, segment, t);
    const std::array<glm::vec3, 4> p = segmentPoints(segment);

    const float s = 1.0f - t;
    return 3.0f * s * s * (p[1] - p[0]) + 6.0f * s * t * (p[2] - p[1]) + 3.0f * t * t * (p[3] - p[2]);
}

float FlightPath::length() const
{
    return m_tableDistances.back();
}

// find the two table entries around the distance and interpolate the 'u' between them
float FlightPath::parameterAtDistance(float distance) const
{
    distance = std::fmod(distance, length());
    if (distance < 0.0f)
        distance += length();

    const auto next = std::upper_bound(m_tableDistances.begin(), m_tableDistances.end(), distance);
    const auto index = static_cast<std::size_t>(next - m_tableDistances.begin());
    const std::size_t i = std::clamp<std::size_t>(index, 1, m_tableDistances.size() - 1);
    const float fraction = (distance - m_tableDistances[i - 1]) / (m_tableDistances[i] - m_tableDistances[i - 1]);
    return m_tableParameters[i - 1] + fraction * (m_tableParameters[i] - m_tableParameters[i - 1]);
}

std::size_t FlightPath::segmentCount() const
{
    return m_joints.size();
}

// segment i goes from joint i to joint i + 1 and the last one goes back to 0
std::array<glm::vec3, 4> FlightPath::segmentPoints(std::size_t segment) const
{
    const Joint& start = m_joints[segment];
    const Joint& end = m_joints[(segment + 1) % m_joints.size()];
    const glm::vec3 handleBeforeEnd = 2.0f * end.position - end.handleAfter;
    return { start.position, start.handleAfter, handleBeforeEnd, end.position };
}

// u is the segment number and the fractional part is the segment-specific t (0-1)
void FlightPath::segmentAndLocal(float u, std::size_t& segment, float& t) const
{
    const float count = static_cast<float>(segmentCount());
    u = std::fmod(u, count);
    if (u < 0.0f)
        u += count;

    const float whole = std::floor(u);
    segment = std::min(static_cast<std::size_t>(whole), segmentCount() - 1);
    t = u - static_cast<float>(segment);
}

// walk along u in i increments, measuring distance traveled
void FlightPath::buildArcLengthTable()
{
    const std::size_t steps = segmentCount() * SAMPLES_PER_SEGMENT;
    m_tableDistances = { 0.0f };
    m_tableParameters = { 0.0f };

    glm::vec3 previous = position(0.0f);
    float distance = 0.0f;
    for (std::size_t i = 1; i <= steps; ++i) {
        const float u = static_cast<float>(i) / SAMPLES_PER_SEGMENT;
        const glm::vec3 current = position(u);
        distance += glm::distance(previous, current);
        m_tableDistances.push_back(distance);
        m_tableParameters.push_back(u);
        previous = current;
    }
}
