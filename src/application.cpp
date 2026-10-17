// Window includes GLFW and OpenGL headers in the required order.
#include <framework/window.h>
#include "mesh.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/trackball.h>
#include <iostream>
#include <stdexcept>
#include <vector>

class Application {
public:
    Application()
        : m_window("Reactor Inspection - Platform Viewer", glm::ivec2(1600, 900), OpenGLVersion::GL41)
        , m_camera(&m_window, glm::radians(CAMERA_FOV_DEGREES))
    {
        m_window.maximize();
        setOverviewCamera();
        m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/platform.obj");

        ShaderBuilder defaultBuilder;
        defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
        defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
        m_defaultShader = defaultBuilder.build();

        // not used for now
        ShaderBuilder shadowBuilder;
        shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
        shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl");
        m_shadowShader = shadowBuilder.build();
    }

    void update()
    {
        while (!m_window.shouldClose()) {
            m_window.updateInput();

            const glm::ivec2 framebufferSize = m_window.getFrameBufferSize();
            if (m_window.isMinimized() || framebufferSize.x <= 0 || framebufferSize.y <= 0) {
                ImGui::EndFrame();
                glfwWaitEventsTimeout(0.1);
                continue;
            }

            glViewport(0, 0, framebufferSize.x, framebufferSize.y);
            drawUi();

            // Read the camera after UI presets have been applied, so they affect this frame.
            const float aspectRatio = static_cast<float>(framebufferSize.x) / static_cast<float>(framebufferSize.y);
            m_viewMatrix = m_camera.viewMatrix();
            m_projectionMatrix = glm::perspective(glm::radians(CAMERA_FOV_DEGREES), aspectRatio, 0.1f, 150.0f);

            renderScene();
            m_window.swapBuffers();
        }
    }

private:
    static constexpr float CAMERA_FOV_DEGREES = 45.0f;

    enum class ShadingMode {
        Lit,
        BaseColor,
        Normals
    };

    void setOverviewCamera()
    {
        // The platform's front (and logo) faces +Z in the exported model.
        m_camera.setCamera(glm::vec3(0.0f, 0.2f, 0.0f),
            glm::radians(glm::vec3(45.0f, 160.0f, 0.0f)), 16.0f);
    }

    void setLogoCamera()
    {
        m_camera.setCamera(glm::vec3(0.0f, 0.38f, 4.4f),
            glm::radians(glm::vec3(55.0f, 180.0f, 0.0f)), 2.8f);
    }

    void drawUi()
    {
        ImGui::Begin("Platform viewer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted("Platform: 12 m diameter");

        if (ImGui::Button("Overview"))
            setOverviewCamera();
        ImGui::SameLine();
        if (ImGui::Button("Logo detail"))
            setLogoCamera();

        int shadingMode = static_cast<int>(m_shadingMode);
        if (ImGui::Combo("Shading", &shadingMode, "Lit\0Base color\0Normals\0"))
            m_shadingMode = static_cast<ShadingMode>(shadingMode);

        ImGui::Separator();
        ImGui::TextUnformatted("Left drag: orbit");
        ImGui::TextUnformatted("Right drag: pan");
        ImGui::TextUnformatted("Mouse wheel: zoom");

        ImGui::Separator();
        ImGui::Text("Display scale: %.2fx", static_cast<double>(m_window.getDpiScalingFactor()));
        float uiScale = m_window.getUiScale();
        if (ImGui::SliderFloat("UI scale", &uiScale, 0.75f, 2.0f, "%.2fx"))
            m_window.setUiScale(uiScale);
        ImGui::End();
    }

    void renderScene()
    {
        // The scene shader encodes sRGB itself; ImGui also supplies display-space colors.
        glDisable(GL_FRAMEBUFFER_SRGB);
        glClearColor(0.84f, 0.87f, 0.90f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

        m_defaultShader.bind();
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
        glUniform3fv(m_defaultShader.getUniformLocation("directionToLight"), 1, glm::value_ptr(m_directionToLight));
        glUniform1i(m_defaultShader.getUniformLocation("shadingMode"), static_cast<int>(m_shadingMode));

        for (const GPUMesh& mesh : m_meshes)
            mesh.draw(m_defaultShader);
    }

    Window m_window;
    Trackball m_camera;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;

    std::vector<GPUMesh> m_meshes;
    ShadingMode m_shadingMode { ShadingMode::Lit };
    glm::vec3 m_directionToLight { -0.4f, 1.0f, 0.6f };

    // Model coordinates are already in meters, centered at ground level.
    glm::mat4 m_projectionMatrix { 1.0f };
    glm::mat4 m_viewMatrix { 1.0f };
    glm::mat4 m_modelMatrix { 1.0f };
};

int main()
{
    try {
        Application app;
        app.update();
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
