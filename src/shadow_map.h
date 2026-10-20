#pragma once

#include <framework/opengl_includes.h>

class ShadowMap {
public:
    void initialize(int resolution);
    void bindForDrawing() const;
    void bindForSampling(GLenum textureUnit) const;

private:
    GLuint m_depthTexture { 0 };
    GLuint m_framebuffer { 0 };
    int m_resolution { 0 };
};
