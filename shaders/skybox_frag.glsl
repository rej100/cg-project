#version 410

uniform samplerCube environmentMap;
in vec3 fragDirection;
layout(location = 0) out vec4 fragColor;

// Match the scene shader: both paths encode linear RGB before writing to the
// current SDR framebuffer, whose automatic sRGB encoding is disabled.
vec3 linearToSrgb(vec3 color)
{
    vec3 result;
    for (int channel = 0; channel < 3; ++channel) {
        float value = max(color[channel], 0.0);
        if (value <= 0.0031308) {
            result[channel] = 12.92 * value;
        } else {
            result[channel] = 1.055 * pow(value, 1.0 / 2.4) - 0.055;
        }
    }
    return result;
}

void main()
{
    // Always use the sharp level for the visible sky. Reflection roughness
    // selects blurrier levels separately in the material shader.
    vec3 color = textureLod(environmentMap, normalize(fragDirection), 0.0).rgb;
    fragColor = vec4(linearToSrgb(color), 1.0);
}
