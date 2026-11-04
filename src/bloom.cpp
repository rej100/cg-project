#include "bloom.h"

// https://learnopengl.com/Advanced-Lighting/Bloom
// basically the same idea but the blur runs at half resolution so the glow is wider at same cost

namespace {

// float colour texture that we can render into and sample with linear filtering
GLuint createColorTexture(glm::ivec2 size, GLint internalFormat)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // the blur would otherwise wrap light from one edge of the screen to the other
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

GLuint createFramebuffer(GLuint colorTexture)
{
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    return framebuffer;
}

}

void Bloom::initialize()
{
    ShaderBuilder brightBuilder;
    brightBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/fullscreen_vert.glsl");
    brightBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/bloom_bright_frag.glsl");
    m_brightShader = brightBuilder.build();

    ShaderBuilder blurBuilder;
    blurBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/fullscreen_vert.glsl");
    blurBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/bloom_blur_frag.glsl");
    m_blurShader = blurBuilder.build();

    ShaderBuilder compositeBuilder;
    compositeBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/fullscreen_vert.glsl");
    compositeBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/bloom_composite_frag.glsl");
    m_compositeShader = compositeBuilder.build();

    // the fullscreen triangle is generated from gl_VertexID but core profile still needs some vao bound
    glGenVertexArrays(1, &m_emptyVao);
}

void Bloom::resize(glm::ivec2 size)
{
    // minimized window has size 0 and we cant make 0 sized textures
    if (size == m_size || size.x <= 0 || size.y <= 0)
        return;

    m_size = size;
    m_blurSize = glm::max(size / 2, glm::ivec2(1));
    deleteTextures();
    createTextures();
}

void Bloom::createTextures()
{
    // the scene gets a float colour texture plus a depth buffer for the depth test
    // also 32 bit because the sun values can be more than 16
    m_sceneTexture = createColorTexture(m_size, GL_RGBA32F);
    m_sceneFramebuffer = createFramebuffer(m_sceneTexture);
    glGenRenderbuffers(1, &m_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_size.x, m_size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);

    for (int i = 0; i < 2; ++i) {
        m_blurTextures[i] = createColorTexture(m_blurSize, GL_RGBA16F);
        m_blurFramebuffers[i] = createFramebuffer(m_blurTextures[i]);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Bloom::deleteTextures()
{
    // deleting 0 is ignored by opengl so this is fine before the first create
    glDeleteFramebuffers(1, &m_sceneFramebuffer);
    glDeleteTextures(1, &m_sceneTexture);
    glDeleteRenderbuffers(1, &m_depthRenderbuffer);
    glDeleteFramebuffers(2, m_blurFramebuffers);
    glDeleteTextures(2, m_blurTextures);
}

void Bloom::bindForDrawing() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebuffer);
    glViewport(0, 0, m_size.x, m_size.y);
}

void Bloom::apply(bool enabled, float strength) const
{
    // fullscreen passes dont need depth and should not blend
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(m_emptyVao);

    if (enabled) {
        glViewport(0, 0, m_blurSize.x, m_blurSize.y);

        // bright pass: keep only the pixels brighter than 1, written at half resolution
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFramebuffers[0]);
        m_brightShader.bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
        glUniform1i(m_brightShader.getUniformLocation("sceneTexture"), 0);
        drawFullscreenTriangle();

        // gaussian blur is separable so do horizontal then vertical, a few times for a wider glow
        m_blurShader.bind();
        glUniform1i(m_blurShader.getUniformLocation("inputTexture"), 0);
        const GLint horizontalLocation = m_blurShader.getUniformLocation("horizontal");
        for (int i = 0; i < BLUR_ITERATIONS; ++i) {
            // horizontal: 0 -> 1
            glBindFramebuffer(GL_FRAMEBUFFER, m_blurFramebuffers[1]);
            glBindTexture(GL_TEXTURE_2D, m_blurTextures[0]);
            glUniform1i(horizontalLocation, GL_TRUE);
            drawFullscreenTriangle();

            // vertical: 1 -> 0
            glBindFramebuffer(GL_FRAMEBUFFER, m_blurFramebuffers[0]);
            glBindTexture(GL_TEXTURE_2D, m_blurTextures[1]);
            glUniform1i(horizontalLocation, GL_FALSE);
            drawFullscreenTriangle();
        }
    }

    // combine onto the screen: scene + strength * blur, then tone map and gamma
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_size.x, m_size.y);
    m_compositeShader.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_blurTextures[0]);
    glUniform1i(m_compositeShader.getUniformLocation("sceneTexture"), 0);
    glUniform1i(m_compositeShader.getUniformLocation("bloomTexture"), 1);
    glUniform1f(m_compositeShader.getUniformLocation("bloomStrength"), enabled ? strength : 0.0f);
    drawFullscreenTriangle();

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void Bloom::drawFullscreenTriangle() const
{
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
