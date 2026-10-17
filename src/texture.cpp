#include "texture.h"
#include <framework/image.h>

#include <algorithm>
#include <utility>
#include <vector>

Texture::Texture(const std::filesystem::path& filePath, TextureColorSpace colorSpace)
    : Texture(Image { filePath }, colorSpace)
{
}

Texture::Texture(const Image& image, TextureColorSpace colorSpace)
{
    // before uploading the texture to the gpu we need to tell opengl if the values are linear rgb or srgb
    // so so if the values in image are srgb, colorSpace needs to be TextureColorSpace:Srgb and vice versa
    if (colorSpace == TextureColorSpace::Srgb && image.channels != 3 && image.channels != 4)
        throw ImageLoadingException("sRGB textures require RGB or RGBA image data");

    GLenum format;
    GLenum internalFormat;
    switch (image.channels) {
    case 1:
        format = GL_RED;
        internalFormat = GL_R8;
        break;
    case 3:
        format = GL_RGB;
        internalFormat = colorSpace == TextureColorSpace::Srgb ? GL_SRGB8 : GL_RGB8;
        break;
    case 4:
        format = GL_RGBA;
        // An sRGB RGBA format decodes RGB only; alpha is not sRGB-encoded.
        internalFormat = colorSpace == TextureColorSpace::Srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        break;
    default:
        throw ImageLoadingException("Unsupported number of image channels");
    }

    // Image stores rows top-to-bottom; OpenGL UVs expect the first row at the bottom.
    const size_t rowBytes = static_cast<size_t>(image.width) * static_cast<size_t>(image.channels);
    const size_t height = static_cast<size_t>(image.height);
    std::vector<uint8_t> bottomUpPixels(rowBytes * height);
    for (size_t y = 0; y < height; ++y) {
        std::copy_n(image.get_data() + (height - 1 - y) * rowBytes,
            rowBytes, bottomUpPixels.data() + y * rowBytes);
    }
    // ad

    // Create a texture on the GPU and bind it for parameter setting
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Set behavior for when texture coordinates are outside the [0, 1] range (wrap around).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Trilinear minification and bilinear magnification.
    // GL_LINEAR is sample interpolation here not rgb encoding
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Rows are tightly packed even when their byte count is not a multiple of four.
    GLint previousAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), image.width, image.height,
        0, format, GL_UNSIGNED_BYTE, bottomUpPixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);

    // Generate mip-maps
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(Texture&& other) noexcept
    : m_texture(std::exchange(other.m_texture, INVALID))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        if (m_texture != INVALID)
            glDeleteTextures(1, &m_texture);
        m_texture = std::exchange(other.m_texture, INVALID);
    }
    return *this;
}

Texture::~Texture()
{
    if (m_texture != INVALID)
        glDeleteTextures(1, &m_texture);
}

void Texture::bind(GLenum textureSlot) const
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}
