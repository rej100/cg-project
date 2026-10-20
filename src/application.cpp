// Window includes GLFW and OpenGL headers in the required order.
#include <framework/window.h>
#include "environment.h"
#include "light.h"
#include "model.h"
#include "shadow_map.h"
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
        : m_window("Reactor Inspection", glm::ivec2(1600, 900), OpenGLVersion::GL41)
        , m_camera(&m_window, glm::radians(CAMERA_FOV_DEGREES))
    {
        m_window.maximize();
        // Filter across cubemap face boundaries instead of leaving visible seams.
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        setOverviewCamera();

        m_environment.initialize(RESOURCE_ROOT "assets/environment/qwantani_noon_puresky_2k.hdr");

        m_platformModel.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/platform.obj");
        m_reactorModel.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/reactor.obj");
        m_drone_bodyModel.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/drone_body.obj");
        m_l_b_propeller.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/L_B_propeller.obj");
        m_l_f_propeller.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/L_F_propeller.obj");
        m_r_b_propeller.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/R_B_propeller.obj");
        m_r_f_propeller.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/R_F_propeller.obj");

        SceneObject platformObject {};
        platformObject.model = &m_platformModel;
        m_sceneObjects.push_back(platformObject);

        SceneObject reactorObject {};
        reactorObject.model = &m_reactorModel;
        reactorObject.modelMatrix = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(0.0f, PLATFORM_HEIGHT, 0.0f));
        m_sceneObjects.push_back(reactorObject);

        m_droneBodyMatrix = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(0.0f, PLATFORM_HEIGHT + 4.0f, 4.0f));
        m_droneBodyMatrix = glm::scale(m_droneBodyMatrix, glm::vec3(4.0f));

        SceneObject droneBodyObject {};
        droneBodyObject.model = &m_drone_bodyModel;
        droneBodyObject.modelMatrix = m_droneBodyMatrix;
        m_sceneObjects.push_back(droneBodyObject);

        SceneObject leftFrontPropellerObject {};
        leftFrontPropellerObject.model = &m_l_f_propeller;
        leftFrontPropellerObject.modelMatrix = glm::translate(m_droneBodyMatrix, m_l_f_propellerOffset);
        m_sceneObjects.push_back(leftFrontPropellerObject);

        SceneObject rightFrontPropellerObject {};
        rightFrontPropellerObject.model = &m_r_f_propeller;
        rightFrontPropellerObject.modelMatrix = glm::translate(m_droneBodyMatrix, m_r_f_propellerOffset);
        m_sceneObjects.push_back(rightFrontPropellerObject);

        SceneObject leftBackPropellerObject {};
        leftBackPropellerObject.model = &m_l_b_propeller;
        leftBackPropellerObject.modelMatrix = glm::translate(m_droneBodyMatrix, m_l_b_propellerOffset);
        m_sceneObjects.push_back(leftBackPropellerObject);

        SceneObject rightBackPropellerObject {};
        rightBackPropellerObject.model = &m_r_b_propeller;
        rightBackPropellerObject.modelMatrix = glm::translate(m_droneBodyMatrix, m_r_b_propellerOffset);
        m_sceneObjects.push_back(rightBackPropellerObject);

        m_spotlightMarkerModel.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/placeholder.obj");
        m_spotlightMarkerObject.model = &m_spotlightMarkerModel;

        ShaderBuilder defaultBuilder;
        defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
        defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
        m_defaultShader = defaultBuilder.build();

        ShaderBuilder shadowBuilder;
        shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
        shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl");
        m_shadowShader = shadowBuilder.build();
        m_sunShadowMap.initialize(SUN_SHADOW_RESOLUTION);
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
    static constexpr float PLATFORM_HEIGHT = 0.38f;
    static constexpr int SUN_SHADOW_RESOLUTION = 2048;

    enum class ShadingMode {
        Lit,
        Pbr,
        BaseColor,
        Normals
    };

    void setOverviewCamera()
    {
        // The platform and reactor face +Z in application coordinates.
        m_camera.setCamera(glm::vec3(0.0f, 1.1f, 0.0f),
            glm::radians(glm::vec3(35.0f, 160.0f, 0.0f)), 16.0f);
    }

    void setReactorCamera()
    {
        m_camera.setCamera(glm::vec3(0.0f, PLATFORM_HEIGHT + 1.75f, 0.0f),
            glm::radians(glm::vec3(20.0f, 155.0f, 0.0f)), 6.5f);
    }

    void setReactorPanelCamera()
    {
        m_camera.setCamera(glm::vec3(0.0f, PLATFORM_HEIGHT + 1.32f, 1.03f),
            glm::radians(glm::vec3(5.0f, 180.0f, 0.0f)), 2.2f);
    }

    void setPlatformLogoCamera()
    {
        m_camera.setCamera(glm::vec3(0.0f, PLATFORM_HEIGHT, 4.4f),
            glm::radians(glm::vec3(55.0f, 180.0f, 0.0f)), 2.8f);
    }

    void drawUi()
    {
        ImGui::Begin("Scene controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted("Platform: 12 m diameter");
        ImGui::TextUnformatted("Reactor: 3.5 m tall");

        if (ImGui::Button("Scene overview"))
            setOverviewCamera();
        ImGui::SameLine();
        if (ImGui::Button("Reactor"))
            setReactorCamera();
        if (ImGui::Button("Reactor panel"))
            setReactorPanelCamera();
        ImGui::SameLine();
        if (ImGui::Button("Platform logo"))
            setPlatformLogoCamera();

        int shadingMode = static_cast<int>(m_shadingMode);
        if (ImGui::Combo("Shading", &shadingMode, "Lit\0PBR\0Base color\0Normals\0"))
            m_shadingMode = static_cast<ShadingMode>(shadingMode);
        ImGui::Checkbox("Emission", &m_enableEmission);
        ImGui::SameLine(ImGui::GetFontSize() * 9.0f);
        ImGui::Checkbox("Material textures", &m_enableMaterialTextures);
        ImGui::Checkbox("Normal mapping", &m_enableNormalMapping);
        ImGui::Checkbox("Environment reflections (PBR)", &m_enableEnvironmentReflections);

        if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
            drawLightingUi();

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

    void drawLightingUi()
    {
        constexpr ImGuiSliderFlags clamped = ImGuiSliderFlags_AlwaysClamp;
        const float intensityOffset = ImGui::GetFontSize() * 7.0f;
        const float sliderWidth = ImGui::GetFontSize() * 8.0f;

        ImGui::Checkbox("Sun", &m_sun.enabled);
        ImGui::SameLine(intensityOffset);
        ImGui::SetNextItemWidth(sliderWidth);
        ImGui::SliderFloat("Intensity##Sun", &m_sun.intensity, 0.0f, 2.0f, "%.2f", clamped);
        ImGui::Checkbox("Sun shadows", &m_enableSunShadows);

        ImGui::Checkbox("Spotlight", &m_spotlight.enabled);
        ImGui::SameLine(intensityOffset);
        ImGui::SetNextItemWidth(sliderWidth);
        ImGui::SliderFloat("Intensity##Spotlight", &m_spotlight.intensity, 0.0f, 2.0f, "%.2f", clamped);

        ImGui::SetNextItemWidth(sliderWidth);
        ImGui::SliderFloat("Ambient strength", &m_ambientStrength, 0.0f, 1.0f, "%.2f", clamped);
        ImGui::Checkbox("Show light marker", &m_showLightMarker);
    }

    void renderSunShadowMap()
    {
        if (!m_sun.enabled || !m_enableSunShadows)
        {
            return;
        }
        if (m_shadingMode != ShadingMode::Lit && m_shadingMode != ShadingMode::Pbr)
        {
            return;
        }


        // so orbiting the viewing camera does not move the shadow projection.
        // we need a camera for the shadow map, but the sun is directional so it doesnt have a world position
        // we just make a camera that is looking at the center of the scene and is some distance away in the direction of the sun
        const glm::vec3 sceneCenter(0.0f, 2.0f, 0.0f);
        const glm::vec3 directionToSun = glm::normalize(m_sun.directionToLight);
        const glm::vec3 up(0.0f, 1.0f, 0.0f);
        const glm::mat4 sunView = glm::lookAt(
            sceneCenter + directionToSun * 20.0f, sceneCenter, up);

        // since the sun is directional we want orhographic projection
        const glm::mat4 sunProjection = glm::ortho(
            -10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 40.0f);
        m_sunViewProjectionMatrix = sunProjection * sunView;

        // save the active framembuffer and the viewport before this function
        GLint previousFramebuffer, previousViewport[4];
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);
        glGetIntegerv(GL_VIEWPORT, previousViewport);
        
        // bind the shadow map framebuffer
        m_sunShadowMap.bindForDrawing();
        // clear everything (so just the depth because shadowmap)
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // bind the shader for drawing the shadowmap
        m_shadowShader.bind();
        const GLint matrixLocation = m_shadowShader.getUniformLocation("mvpMatrix");

        // draw the depth of the scene from the perspective of the light camera into the shadowmap texture
        for (const SceneObject& object : m_sceneObjects) {
            const glm::mat4 lightMvp = m_sunViewProjectionMatrix * object.modelMatrix;
            glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, glm::value_ptr(lightMvp));
            for (const GPUMesh& mesh : object.model->meshes)
                mesh.drawGeometry();
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    }

    void renderScene()
    {
        // enable depth testing before the shadowmap pass
        glEnable(GL_DEPTH_TEST);
        renderSunShadowMap();

        // ImGui supplies srgb so we make shader supply srgb as well
        // and disable automatic encoding for the framebuffer
        glDisable(GL_FRAMEBUFFER_SRGB);
        // the skybox paints over this color 
        glClearColor(0.84f, 0.87f, 0.90f, 1.0f);
        // depth still needs clearing each frame.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw the background sky
        m_environment.drawBackground(m_viewMatrix, m_projectionMatrix);

        const glm::mat4 viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;

        // bind everything for the main shader
        m_defaultShader.bind();
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("viewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));
        uploadLightUniforms();
        glUniform1i(m_defaultShader.getUniformLocation("shadingMode"), static_cast<int>(m_shadingMode));
        glUniform1i(m_defaultShader.getUniformLocation("enableEmission"), m_enableEmission);
        glUniform1i(m_defaultShader.getUniformLocation("enableMaterialTextures"), m_enableMaterialTextures);
        glUniform1i(m_defaultShader.getUniformLocation("enableNormalMapping"), m_enableNormalMapping);
        glUniform1i(m_defaultShader.getUniformLocation("enableEnvironmentReflections"), m_enableEnvironmentReflections);
        glUniform1i(m_defaultShader.getUniformLocation("environmentMap"), 3);
        glUniform1f(m_defaultShader.getUniformLocation("environmentMaxLOD"), m_environment.maxReflectionLod());
        m_environment.bind(GL_TEXTURE3);
        glUniform1i(m_defaultShader.getUniformLocation("enableSunShadows"), m_enableSunShadows);
        glUniform1i(m_defaultShader.getUniformLocation("sunShadowMap"), 4);
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("sunViewProjectionMatrix"),
            1, GL_FALSE, glm::value_ptr(m_sunViewProjectionMatrix));
        m_sunShadowMap.bindForSampling(GL_TEXTURE4);
        glUniform3fv(m_defaultShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(m_camera.position()));

        for (const SceneObject& object : m_sceneObjects)
            drawObject(object);

        if (m_showLightMarker)
            drawSpotlightMarker();
    }

    void uploadLightUniforms()
    {
        glUniform1f(m_defaultShader.getUniformLocation("ambientStrength"), m_ambientStrength);

        glUniform3fv(m_defaultShader.getUniformLocation("sun.directionToLight"), 1, glm::value_ptr(m_sun.directionToLight));
        glUniform3fv(m_defaultShader.getUniformLocation("sun.color"), 1, glm::value_ptr(m_sun.color));
        glUniform1f(m_defaultShader.getUniformLocation("sun.intensity"), m_sun.intensity);
        glUniform1i(m_defaultShader.getUniformLocation("sun.enabled"), m_sun.enabled);

        const glm::vec3 spotlightDirection = m_spotlight.direction();
        glUniform3fv(m_defaultShader.getUniformLocation("spotlight.position"), 1, glm::value_ptr(m_spotlight.position));
        glUniform3fv(m_defaultShader.getUniformLocation("spotlight.direction"), 1, glm::value_ptr(spotlightDirection));
        glUniform3fv(m_defaultShader.getUniformLocation("spotlight.color"), 1, glm::value_ptr(m_spotlight.color));
        glUniform1f(m_defaultShader.getUniformLocation("spotlight.intensity"), m_spotlight.intensity);
        glUniform1f(m_defaultShader.getUniformLocation("spotlight.coneCos"), glm::cos(glm::radians(m_spotlight.coneHalfAngleDegrees)));
        glUniform1i(m_defaultShader.getUniformLocation("spotlight.enabled"), m_spotlight.enabled);
    }

    void drawObject(const SceneObject& object)
    {
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(object.modelMatrix));
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(object.modelMatrix));
        glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));

        for (const GPUMesh& mesh : object.model->meshes)
            mesh.draw(m_defaultShader);
    }

    void drawSpotlightMarker()
    {
        const glm::vec3 direction = m_spotlight.direction();
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(direction, up)) > 0.99f)
            up = glm::vec3(0.0f, 0.0f, 1.0f);

        // Inverting the light's view maps the marker's local -Z onto its aim.
        m_spotlightMarkerObject.modelMatrix = glm::inverse(glm::lookAt(
            m_spotlight.position, m_spotlight.position + direction, up));

        // Base color keeps the debug marker visible independently of lighting.
        const GLint shadingModeLocation = m_defaultShader.getUniformLocation("shadingMode");
        glUniform1i(shadingModeLocation, static_cast<int>(ShadingMode::BaseColor));
        drawObject(m_spotlightMarkerObject);
        glUniform1i(shadingModeLocation, static_cast<int>(m_shadingMode));
    }

    Window m_window;
    Trackball m_camera;
    Environment m_environment;
    ShadowMap m_sunShadowMap;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;

    Model m_platformModel;
    Model m_reactorModel;
    Model m_drone_bodyModel;
    Model m_l_b_propeller;
    Model m_l_f_propeller;
    Model m_r_b_propeller;
    Model m_r_f_propeller;
    glm::mat4 m_droneBodyMatrix { 1.0f };
    // Propeller positions relative to the drone body:
    glm::vec3 m_l_f_propellerOffset { -0.1074f, -0.0204f, -0.0957f };
    glm::vec3 m_r_f_propellerOffset { 0.1072f, -0.0204f, -0.0957f };
    glm::vec3 m_l_b_propellerOffset { -0.1074f, -0.0204f, 0.0915f };
    glm::vec3 m_r_b_propellerOffset { 0.1072f, -0.0204f, 0.0915f };

    std::vector<SceneObject> m_sceneObjects;
    Model m_spotlightMarkerModel;
    SceneObject m_spotlightMarkerObject {};
    bool m_showLightMarker { true };
    ShadingMode m_shadingMode { ShadingMode::Lit };
    bool m_enableEmission { true };
    bool m_enableMaterialTextures { true };
    bool m_enableNormalMapping { true };
    bool m_enableEnvironmentReflections { true };
    bool m_enableSunShadows { true };
    DirectionalLight m_sun;
    SpotLight m_spotlight;
    float m_ambientStrength { 0.25f };

    glm::mat4 m_projectionMatrix { 1.0f };
    glm::mat4 m_viewMatrix { 1.0f };
    glm::mat4 m_sunViewProjectionMatrix { 1.0f };
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
