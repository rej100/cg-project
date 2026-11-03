#include <framework/window.h>
#include "bloom.h"
#include "environment.h"
#include "flight_path.h"
#include "light.h"
#include "model.h"
#include "particle_system.h"
#include "shadow_map.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/trackball.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

class Application {
public:
    Application()
        : m_window("Final Project", glm::ivec2(1600, 900), OpenGLVersion::GL41)
        , m_camera(&m_window, glm::radians(CAMERA_FOV_DEGREES))
    {
        m_window.maximize();
        // for the cubemap face boundaries
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
        m_showcaseModel.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/showcase_model.obj");
        m_lampModel.meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "assets/models/lamp.obj");

        SceneObject platformObject {};
        platformObject.model = &m_platformModel;
        m_sceneObjects.push_back(platformObject);

        SceneObject reactorObject {};
        reactorObject.model = &m_reactorModel;
        reactorObject.modelMatrix = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(0.0f, PLATFORM_HEIGHT, 0.0f));
        m_sceneObjects.push_back(reactorObject);

        // see flight_path for exact joint coords
        m_flightPath.initialize();
        updateDroneMatrix();

        m_droneBodyIndex = m_sceneObjects.size();
        SceneObject droneBodyObject {};
        droneBodyObject.model = &m_drone_bodyModel;
        droneBodyObject.modelMatrix = m_droneBodyMatrix;
        m_sceneObjects.push_back(droneBodyObject);

        m_firstPropellerIndex = m_sceneObjects.size();

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

        // the polished chrome sphere beside the reactor shows environment reflections clearly
        SceneObject showcaseObject {};
        showcaseObject.model = &m_showcaseModel;
        showcaseObject.modelMatrix = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(3.2f, PLATFORM_HEIGHT, 1.5f));
        m_sceneObjects.push_back(showcaseObject);

        const float lampAngle = std::atan2(-LAMP_BASE.x, -LAMP_BASE.z);
        SceneObject lampObject {};
        lampObject.model = &m_lampModel;
        lampObject.modelMatrix = glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), LAMP_BASE), lampAngle, glm::vec3(0.0f, 1.0f, 0.0f)),
            glm::vec3(LAMP_SCALE));
        m_sceneObjects.push_back(lampObject);

        // moving light in frnt of the glass
        m_spotlight.position = glm::vec3(lampObject.modelMatrix * glm::vec4(0.0f, LAMP_GLASS_HEIGHT, LAMP_LIGHT_DEPTH, 1.0f));

        ShaderBuilder defaultBuilder;
        defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
        defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
        m_defaultShader = defaultBuilder.build();

        ShaderBuilder shadowBuilder;
        shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
        shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl");
        m_shadowShader = shadowBuilder.build();
        m_sunShadowMap.initialize(SUN_SHADOW_RESOLUTION);
        m_spotlightShadowMap.initialize(SPOTLIGHT_SHADOW_RESOLUTION);

        // put the emitter at the top of the core on top of the reactor (y 3.5)
        m_particleSystem.initialize(glm::vec3(0.0f, PLATFORM_HEIGHT + 3.52f, 0.0f));

        m_bloom.initialize();
    }

    void update()
    {
        double previousTime = glfwGetTime();
        while (!m_window.shouldClose()) {
            m_window.updateInput();
            const double currentTime = glfwGetTime();
            const float deltaTime = static_cast<float>(currentTime - previousTime);
            previousTime = currentTime;

            const glm::ivec2 framebufferSize = m_window.getFrameBufferSize();
            // the bloom textures have to match the window so recreate them after a resize
            m_bloom.resize(framebufferSize);

            glViewport(0, 0, framebufferSize.x, framebufferSize.y);
            drawUi();
            if (m_enableParticles)
                m_particleSystem.update(deltaTime);

            // distance along the path = previous distance + speed * time since last frame (dt)
            if (m_flyDrone)
                m_droneDistance = std::fmod(m_droneDistance + DRONE_SPEED * deltaTime, m_flightPath.length());
            updateDroneMatrix();
            m_sceneObjects[m_droneBodyIndex].modelMatrix = m_droneBodyMatrix;

            // propller angle = old angle + speed * dt
            m_propellerAngle = std::fmod(m_propellerAngle + PROPELLER_SPEED * deltaTime, glm::two_pi<float>());
            updatePropellerMatrices();

            const float aspectRatio = static_cast<float>(framebufferSize.x) / static_cast<float>(framebufferSize.y);
            m_viewMatrix = cameraViewMatrix();
            m_cameraPosition = glm::vec3(glm::inverse(m_viewMatrix)[3]);
            m_projectionMatrix = glm::perspective(glm::radians(CAMERA_FOV_DEGREES), aspectRatio, 0.1f, 150.0f);

            renderScene(framebufferSize.y);
            m_window.swapBuffers();
        }
    }

private:
    static constexpr float CAMERA_FOV_DEGREES = 45.0f;
    static constexpr float PROPELLER_SPEED = 30.0f;
    static constexpr float DRONE_SPEED = 2.0f;
    static constexpr float DRONE_SCALE = 4.0f;
    static constexpr float PLATFORM_HEIGHT = 0.38f;
    static constexpr glm::vec3 LAMP_BASE { -4.7f, PLATFORM_HEIGHT, 1.0f };
    // the model is about 6 m tall, this makes it about as tall as the reactor
    static constexpr float LAMP_SCALE = 0.6f;
    // in model units: centre of the lamp glass above the lamp base, the glass itself is at local z 0.229
    static constexpr float LAMP_GLASS_HEIGHT = 5.62f;
    static constexpr float LAMP_LIGHT_DEPTH = 0.3f;
    static constexpr int SUN_SHADOW_RESOLUTION = 2048;
    static constexpr int SPOTLIGHT_SHADOW_RESOLUTION = 2048;

    enum class ShadingMode {
        Pbr,
        BaseColor
    };

    enum class CameraMode {
        Overview,
        Follow,  
        Top
    };

    void updateDroneMatrix()
    {
        const float u = m_flightPath.parameterAtDistance(m_droneDistance);
        m_dronePosition = m_flightPath.position(u);
        m_droneForward = glm::normalize(m_flightPath.tangent(u));
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        const glm::vec3 right = glm::normalize(glm::cross(m_droneForward, worldUp));
        m_droneUp = glm::cross(right, m_droneForward);

        // matrix columns are where the model's 3 axes end up
        // the front propellers are at -Z so the model -Z becomes forward
        const glm::mat4 orientation(
            glm::vec4(right, 0.0f),
            glm::vec4(m_droneUp, 0.0f),
            glm::vec4(-m_droneForward, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

        m_droneBodyMatrix = glm::translate(glm::mat4(1.0f), m_dronePosition) * orientation
            * glm::scale(glm::mat4(1.0f), glm::vec3(DRONE_SCALE));
    }

    glm::mat4 cameraViewMatrix() const
    {
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        switch (m_cameraMode) {
        case CameraMode::Follow: {
            const glm::vec3 eye = m_dronePosition - 3.0f * m_droneForward + 1.2f * m_droneUp;
            return glm::lookAt(eye, m_dronePosition, worldUp);
        }
        case CameraMode::Top:
            return glm::lookAt(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        case CameraMode::Overview:
        default:
            return m_camera.viewMatrix();
        }
    }

    void updatePropellerMatrices()
    {
        // first body then offset and last spin around propellers own Y axis
        const glm::vec3 offsets[] = { m_l_f_propellerOffset, m_r_f_propellerOffset, m_l_b_propellerOffset, m_r_b_propellerOffset };
        for (size_t i = 0; i < 4; ++i) {
            glm::mat4 matrix = glm::translate(m_droneBodyMatrix, offsets[i]);
            float angle = m_propellerAngle;
            if (i==1 || i==2){
                angle = -m_propellerAngle;
            }
            matrix = glm::rotate(matrix, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            m_sceneObjects[m_firstPropellerIndex + i].modelMatrix = matrix;
        }
    }

    void setOverviewCamera()
    {
        // the platform and reactor face z
        m_camera.setCamera(glm::vec3(0.0f, 1.1f, 0.0f),
            glm::radians(glm::vec3(35.0f, 160.0f, 0.0f)), 16.0f);
    }

    void setReactorPanelCamera()
    {
        m_camera.setCamera(glm::vec3(0.0f, PLATFORM_HEIGHT + 1.32f, 1.03f),
            glm::radians(glm::vec3(5.0f, 180.0f, 0.0f)), 2.2f);
    }

    void setReflectionShowcaseCamera()
    {
        m_camera.setCamera(glm::vec3(3.2f, PLATFORM_HEIGHT + 1.1f, 1.5f),
            glm::radians(glm::vec3(15.0f, 170.0f, 0.0f)), 2.6f);
    }

    void drawUi()
    {
        ImGui::Begin("Scene controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::CollapsingHeader("Multiple viewpoints", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Scene overview")) {
                setOverviewCamera();
                m_cameraMode = CameraMode::Overview;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reactor panel")) {
                setReactorPanelCamera();
                m_cameraMode = CameraMode::Overview;
            }
            if (ImGui::Button("Environment mapping")) {
                setReflectionShowcaseCamera();
                m_cameraMode = CameraMode::Overview;
            }
            ImGui::SameLine();
            if (ImGui::Button("Follow drone"))
                m_cameraMode = CameraMode::Follow;
            if (ImGui::Button("Top view"))
                m_cameraMode = CameraMode::Top;
        }

        if (ImGui::CollapsingHeader("Bezier curves (constant speed)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Show path", &m_showPath);
            ImGui::Checkbox("Fly", &m_flyDrone);
        }

        if (ImGui::CollapsingHeader("Advanced shading: PBR", ImGuiTreeNodeFlags_DefaultOpen)) {
            int shadingMode = static_cast<int>(m_shadingMode);
            if (ImGui::Combo("Shading", &shadingMode, "PBR\0Base color\0"))
                m_shadingMode = static_cast<ShadingMode>(shadingMode);
            ImGui::Checkbox("Emission", &m_enableEmission);
        }

        if (ImGui::CollapsingHeader("Material textures##Section", ImGuiTreeNodeFlags_DefaultOpen))
            ImGui::Checkbox("Material textures", &m_enableMaterialTextures);

        if (ImGui::CollapsingHeader("Normal mapping##Section", ImGuiTreeNodeFlags_DefaultOpen))
            ImGui::Checkbox("Normal mapping", &m_enableNormalMapping);

        if (ImGui::CollapsingHeader("Environment mapping##Section", ImGuiTreeNodeFlags_DefaultOpen))
            ImGui::Checkbox("Environment reflections", &m_enableEnvironmentReflections);

        if (ImGui::CollapsingHeader("Shadows from multiple light sources", ImGuiTreeNodeFlags_DefaultOpen))
            drawLightingUi();

        if (ImGui::CollapsingHeader("Particle effects", ImGuiTreeNodeFlags_DefaultOpen))
            ImGui::Checkbox("Particles", &m_enableParticles);

        if (ImGui::CollapsingHeader("Bloom##Section", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Bloom", &m_enableBloom);
            ImGui::SliderFloat("Strength##Bloom", &m_bloomStrength, 0.0f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        }

        if (ImGui::CollapsingHeader("UI settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Display scale: %.2fx", static_cast<double>(m_window.getDpiScalingFactor()));
            float uiScale = m_window.getUiScale();
            if (ImGui::SliderFloat("UI scale", &uiScale, 0.75f, 2.0f, "%.2fx"))
                m_window.setUiScale(uiScale);
        }
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
        ImGui::Checkbox("Spotlight shadows", &m_enableSpotlightShadows);

        ImGui::SetNextItemWidth(sliderWidth);
        ImGui::SliderFloat("Ambient strength", &m_ambientStrength, 0.0f, 1.0f, "%.2f", clamped);
    }

    void renderSunShadowMap()
    {
        if (!m_sun.enabled || !m_enableSunShadows)
        {
            return;
        }
        if (m_shadingMode != ShadingMode::Pbr)
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

        renderShadowMap(m_sunShadowMap, m_sunViewProjectionMatrix);
    }

    void renderSpotlightShadowMap()
    {
        if (!m_spotlight.enabled || !m_enableSpotlightShadows)
        {
            return;
        }
        if (m_shadingMode != ShadingMode::Pbr)
        {
            return;
        }

        const glm::vec3 up(0.0f, 1.0f, 0.0f);
        const glm::mat4 spotlightView = glm::lookAt(m_spotlight.position, m_spotlight.position + m_spotlight.direction(), up);
        const glm::mat4 spotlightProjection = glm::perspective(
            glm::radians(m_spotlight.coneHalfAngleDegrees * 2.0f), 1.0f, 0.5f, 30.0f);

        m_spotlightViewProjectionMatrix = spotlightProjection * spotlightView;

        renderShadowMap(m_spotlightShadowMap, m_spotlightViewProjectionMatrix);
    }

    void renderShadowMap(const ShadowMap& shadowMap, const glm::mat4& lightViewProjectionMatrix)
    {
        // save the active framembuffer and the viewport before this function
        GLint previousFramebuffer, previousViewport[4];
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);
        glGetIntegerv(GL_VIEWPORT, previousViewport);
        
        // bind the shadow map framebuffer
        shadowMap.bindForDrawing();
        // clear everything (so just the depth because shadowmap)
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // bind the shader for drawing the shadowmap
        m_shadowShader.bind();
        const GLint matrixLocation = m_shadowShader.getUniformLocation("mvpMatrix");

        // draw the depth of the scene from the perspective of the light camera into the shadowmap texture
        for (const SceneObject& object : m_sceneObjects) {
            const glm::mat4 lightMvp = lightViewProjectionMatrix * object.modelMatrix;
            glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, glm::value_ptr(lightMvp));
            for (const GPUMesh& mesh : object.model->meshes)
                mesh.drawGeometry();
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    }

    void renderScene(int framebufferHeight)
    {
        // enable depth testing before the shadowmap passes
        glEnable(GL_DEPTH_TEST);
        renderSunShadowMap();
        renderSpotlightShadowMap();

        // the scene goes into the float framebuffer in linear rgb, the bloom composite converts to srgb itself
        // (ImGui supplies srgb too) so disable automatic encoding for the framebuffer
        m_bloom.bindForDrawing();
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
        glUniform1i(m_defaultShader.getUniformLocation("enableSpotlightShadows"), m_enableSpotlightShadows);
        glUniform1i(m_defaultShader.getUniformLocation("spotlightShadowMap"), 5);
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("spotlightViewProjectionMatrix"),
            1, GL_FALSE, glm::value_ptr(m_spotlightViewProjectionMatrix));
        m_spotlightShadowMap.bindForSampling(GL_TEXTURE5);
        glUniform3fv(m_defaultShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(m_cameraPosition));

        for (const SceneObject& object : m_sceneObjects)
            drawObject(object);

        // draw the path after the scene because it binds its own shader
        if (m_showPath)
            m_flightPath.draw(viewProjectionMatrix);

        if (m_enableParticles)
            m_particleSystem.draw(m_viewMatrix, m_projectionMatrix, framebufferHeight);

        // onto the screen, ImGui draws on top of this afterwards
        m_bloom.apply(m_enableBloom, m_bloomStrength);
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

    Window m_window;
    Trackball m_camera;
    Environment m_environment;
    ParticleSystem m_particleSystem;
    FlightPath m_flightPath;
    ShadowMap m_sunShadowMap;
    ShadowMap m_spotlightShadowMap;
    Bloom m_bloom;

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
    Model m_showcaseModel;
    Model m_lampModel;
    glm::mat4 m_droneBodyMatrix { 1.0f };
    // propeller positions relative to the drone body
    glm::vec3 m_l_f_propellerOffset { -0.1074f, -0.0204f, -0.0957f };
    glm::vec3 m_r_f_propellerOffset { 0.1072f, -0.0204f, -0.0957f };
    glm::vec3 m_l_b_propellerOffset { -0.1074f, -0.0204f, 0.0915f };
    glm::vec3 m_r_b_propellerOffset { 0.1072f, -0.0204f, 0.0915f };
    size_t m_firstPropellerIndex { 0 };
    float m_propellerAngle { 0.0f };
    size_t m_droneBodyIndex { 0 };
    float m_droneDistance { 0.0f }; // metres travelled along the path
    glm::vec3 m_dronePosition { 0.0f };
    glm::vec3 m_droneForward { 0.0f, 0.0f, -1.0f };
    glm::vec3 m_droneUp { 0.0f, 1.0f, 0.0f };
    bool m_showPath { true };
    bool m_flyDrone { false };
    CameraMode m_cameraMode { CameraMode::Overview };

    std::vector<SceneObject> m_sceneObjects;
    ShadingMode m_shadingMode { ShadingMode::Pbr };
    bool m_enableEmission { true };
    bool m_enableMaterialTextures { true };
    bool m_enableNormalMapping { true };
    bool m_enableEnvironmentReflections { true };
    bool m_enableSunShadows { true };
    bool m_enableSpotlightShadows { true };
    bool m_enableParticles { true };
    bool m_enableBloom { true };
    float m_bloomStrength { 0.6f };
    DirectionalLight m_sun;
    SpotLight m_spotlight;
    float m_ambientStrength { 0.25f };

    glm::mat4 m_projectionMatrix { 1.0f };
    glm::mat4 m_viewMatrix { 1.0f };
    glm::vec3 m_cameraPosition { 0.0f }; // of the active camera mode, for the specular and reflections
    glm::mat4 m_sunViewProjectionMatrix { 1.0f };
    glm::mat4 m_spotlightViewProjectionMatrix { 1.0f };
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
