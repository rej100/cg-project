#pragma once

#include <framework/opengl_includes.h>
#include <framework/shader.h>
#include <glm/vec2.hpp>

// the scene is rendered into a float framebuffer so colours above 1 survive.
// the bright parts get blurred and added back on top before tone mapping to the screen
class Bloom {
public:
    // call once after creating the OpenGL context
    void initialize();
    // (re)creates the textures when the framebuffer size changed
    void resize(glm::ivec2 size);
    // bind the float framebuffer so the scene renders into it
    void bindForDrawing() const;
    // bright pass, blur, and combine everything onto the default framebuffer
    void apply(bool enabled, float strength) const;

private:
    void createTextures();
    void deleteTextures();
    void drawFullscreenTriangle() const;

    static constexpr int BLUR_ITERATIONS = 5;

    Shader m_brightShader;
    Shader m_blurShader;
    Shader m_compositeShader;
    GLuint m_emptyVao { 0 };

    glm::ivec2 m_size { 0 };
    glm::ivec2 m_blurSize { 0 };
    GLuint m_sceneFramebuffer { 0 };
    GLuint m_sceneTexture { 0 };
    GLuint m_depthRenderbuffer { 0 };
    // the blur ping-pongs between these two, the result always ends up in index 0
    GLuint m_blurFramebuffers[2] { 0, 0 };
    GLuint m_blurTextures[2] { 0, 0 };
};
