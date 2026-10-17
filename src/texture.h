#pragma once
#include <filesystem>
#include <framework/opengl_includes.h>
#include <stdexcept>

struct Image;

// what encoding are the rgb value sof the source image
enum class TextureColorSpace {
    Linear, // already linear rgb
    Srgb    // srgb
};

struct ImageLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Texture {
public:
    explicit Texture(const std::filesystem::path& filePath, TextureColorSpace colorSpace = TextureColorSpace::Linear);
    explicit Texture(const Image& image, TextureColorSpace colorSpace = TextureColorSpace::Linear);
    Texture(const Texture&) = delete;
    Texture(Texture&&) noexcept;
    ~Texture();

    Texture& operator=(const Texture&) = delete;
    Texture& operator=(Texture&&) noexcept;

    void bind(GLenum textureSlot) const;

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_texture { INVALID };
};
