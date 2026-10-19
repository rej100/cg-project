#include "environment.h"
#include <framework/image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

// if enabled is true, enables the capability in opengl. otherwise disables it
void restoreEnable(GLenum capability, GLboolean enabled)
{
    if (enabled)
        glEnable(capability);
    else
        glDisable(capability);
}

// opengl state that we change when initalising the environment
struct OpenGLState {
    GLint framebuffer, program, vao, arrayBuffer, activeTexture, texture2D, textureCube, viewport[4];
    GLboolean depthTest, cullFace, blend, scissorTest, framebufferSrgb;
};

// returns the current state of the opengl values we will want to restore
OpenGLState readOpenGLState()
{
    OpenGLState state {};
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &state.framebuffer);
    glGetIntegerv(GL_CURRENT_PROGRAM, &state.program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &state.vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &state.arrayBuffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &state.activeTexture);
    glGetIntegerv(GL_VIEWPORT, state.viewport);
    state.depthTest = glIsEnabled(GL_DEPTH_TEST);
    state.cullFace = glIsEnabled(GL_CULL_FACE);
    state.blend = glIsEnabled(GL_BLEND);
    state.scissorTest = glIsEnabled(GL_SCISSOR_TEST);
    state.framebufferSrgb = glIsEnabled(GL_FRAMEBUFFER_SRGB);

    // we use texture 0 for the initialisation so save what it had before
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &state.texture2D);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &state.textureCube);
    glActiveTexture(static_cast<GLenum>(state.activeTexture));
    return state;
}

// sets the opengl values to the ones in state
void restoreOpenGLState(const OpenGLState& state)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(state.framebuffer));
    glUseProgram(static_cast<GLuint>(state.program));
    glBindVertexArray(static_cast<GLuint>(state.vao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(state.arrayBuffer));
    glViewport(state.viewport[0], state.viewport[1], state.viewport[2], state.viewport[3]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(state.texture2D));
    glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(state.textureCube));
    glActiveTexture(static_cast<GLenum>(state.activeTexture));
    restoreEnable(GL_DEPTH_TEST, state.depthTest);
    restoreEnable(GL_CULL_FACE, state.cullFace);
    restoreEnable(GL_BLEND, state.blend);
    restoreEnable(GL_SCISSOR_TEST, state.scissorTest);
    restoreEnable(GL_FRAMEBUFFER_SRGB, state.framebufferSrgb);
}

// the coordinates for the vertices of a cube with side length 2 centered at the origin (we use for the cubemap)
const float cubeVertices[] = {
    -1, -1, -1,  1, -1, -1,  1,  1, -1,
     1,  1, -1, -1,  1, -1, -1, -1, -1,
    -1, -1,  1,  1, -1,  1,  1,  1,  1,
     1,  1,  1, -1,  1,  1, -1, -1,  1,
    -1,  1,  1, -1,  1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,  1, -1,  1,  1,
     1,  1,  1,  1,  1, -1,  1, -1, -1,
     1, -1, -1,  1, -1,  1,  1,  1,  1,
    -1, -1, -1,  1, -1, -1,  1, -1,  1,
     1, -1,  1, -1, -1,  1, -1, -1, -1,
    -1,  1, -1,  1,  1, -1,  1,  1,  1,
     1,  1,  1, -1,  1,  1, -1,  1, -1
};

}

void Environment::initialize(const std::filesystem::path& panoramaPath)
{
    // load the sky
    const HDRImage panorama(panoramaPath);

    // this is the shader for generating the faces of the cubemap. we put a camera at the center of the cubemap and for
    // each of the 6 faces, we point it at it and use this to generate the texture.
    ShaderBuilder conversionBuilder;
    conversionBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl");
    conversionBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/environment_convert_frag.glsl");
    Shader conversionShader = conversionBuilder.build();

    ShaderBuilder skyboxBuilder;
    skyboxBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl");
    skyboxBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/skybox_frag.glsl");
    m_skyboxShader = skyboxBuilder.build();

    const OpenGLState previousState = readOpenGLState();
    glActiveTexture(GL_TEXTURE0);
    GLuint panoramaTexture = 0;
    GLuint captureFramebuffer = 0;

    // stb stores the top row first. Flip rows for the conversion shader's
    // bottom-up UVs; positive Y must sample the top (zenith) of the image.
    const size_t rowValues = static_cast<size_t>(panorama.width) * HDRImage::channels;
    const size_t height = static_cast<size_t>(panorama.height);
    std::vector<float> bottomUpPixels(rowValues * height);
    for (size_t row = 0; row < height; ++row) {
        std::copy_n(panorama.get_data() + (height - 1 - row) * rowValues,
            rowValues, bottomUpPixels.data() + row * rowValues);
    }

    glGenTextures(1, &panoramaTexture);
    glBindTexture(GL_TEXTURE_2D, panoramaTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, panorama.width, panorama.height,
        0, GL_RGB, GL_FLOAT, bottomUpPixels.data());
    // Left/right edges join at the panorama's longitude seam; top/bottom
    // are different poles and must not wrap into one another.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &m_cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);
    for (unsigned int face = 0; face < 6; ++face) {
        // This HDRI's sun exceeds the half-float maximum (65504). Keep
        // 32-bit floats so conversion does not clip it before tone mapping.
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB32F,
            FACE_SIZE, FACE_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenVertexArrays(1, &m_cubeVao);
    glGenBuffers(1, &m_cubeVbo);
    glBindVertexArray(m_cubeVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    // Each view looks from the cube center at one face. These up vectors
    // match OpenGL's cubemap face orientation, including the two Y faces.
    const std::array<glm::mat4, 6> captureViews = {
        glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0, -1,  0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0, -1,  0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0,  0,  1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0,  0, -1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0, -1,  0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0, -1,  0))
    };
    const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    glGenFramebuffers(1, &captureFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, captureFramebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0, 0, FACE_SIZE, FACE_SIZE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // The camera sees the cube's inside faces.
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB); // Both panorama and cubemap are linear.
    conversionShader.bind();
    glUniform1i(conversionShader.getUniformLocation("panoramaMap"), 0);
    const GLint matrixLocation = conversionShader.getUniformLocation("viewProjectionMatrix");
    for (unsigned int face = 0; face < 6; ++face) {
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_cubemap, 0);
        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("cubemap frambufer bad");

        const glm::mat4 viewProjection = captureProjection * captureViews[face];
        glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // Ordinary mipmaps give a cheap blur approximation, not the GGX
    // convolution and BRDF integration used by full specular IBL.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    m_maxReflectionLod = std::log2(static_cast<float>(FACE_SIZE));

    glDeleteFramebuffers(1, &captureFramebuffer);
    glDeleteTextures(1, &panoramaTexture);
    restoreOpenGLState(previousState);

    std::cout << "Loaded environment: " << panoramaPath.string() << " ("
              << panorama.width << " x " << panorama.height << " HDR, "
              << FACE_SIZE << " x " << FACE_SIZE << " cubemap faces)\n";
}

void Environment::bind(GLenum textureSlot) const
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);
}

void Environment::drawBackground(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const
{
    const GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean cullFace = glIsEnabled(GL_CULL_FACE);
    GLboolean depthWrite;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
    // Paint the sky first without recording any depth, so even objects farther
    // away than this small helper cube can be drawn over it afterwards.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    // Keeping only view rotation makes the cube follow camera translation.
    // Moving cannot approach the sky; turning still changes the lookup direction.
    const glm::mat4 viewProjection = projectionMatrix * glm::mat4(glm::mat3(viewMatrix));
    m_skyboxShader.bind();
    glUniformMatrix4fv(m_skyboxShader.getUniformLocation("viewProjectionMatrix"),
        1, GL_FALSE, glm::value_ptr(viewProjection));
    glUniform1i(m_skyboxShader.getUniformLocation("environmentMap"), 3);
    bind(GL_TEXTURE3);
    glBindVertexArray(m_cubeVao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthMask(depthWrite);
    restoreEnable(GL_DEPTH_TEST, depthTest);
    restoreEnable(GL_CULL_FACE, cullFace);
}

float Environment::maxReflectionLod() const
{
    return m_maxReflectionLod;
}
