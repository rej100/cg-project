#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()

struct DirectionalLight {
    glm::vec3 directionToLight { -0.4f, 1.0f, 0.6f };
    glm::vec3 color { 1.0f }; // linear rgb
    float intensity { 0.75f };
    bool enabled { true };
};

struct SpotLight {
    glm::vec3 position { 5.0f, 6.0f, 4.5f };
    glm::vec3 target { 0.0f, 1.8f, 0.0f };
    glm::vec3 color { 1.0f }; // linear rgb
    float intensity { 20.0f };
    float innerAngleDegrees { 18.0f };
    float outerAngleDegrees { 25.0f };
    bool enabled { true };

    glm::vec3 direction() const
    {
        const glm::vec3 toTarget = target - position;

        // if target is light aim down
        if (glm::dot(toTarget, toTarget) < 0.000001f)
        {
            return glm::vec3(0.0f, -1.0f, 0.0f);
        }

        return glm::normalize(toTarget);
    }
};
