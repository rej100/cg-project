#pragma once

#include "mesh.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
DISABLE_WARNINGS_POP()
#include <vector>

struct Model {
    std::vector<GPUMesh> meshes;
};

struct SceneObject {
    const Model* model;
    glm::mat4 modelMatrix { 1.0f };
};
