#version 410

// scene + strength * blurred bright parts, then tone map back to 0..1 and convert to srgb for the screen

uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform float bloomStrength;

in vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

// aces filmic curve fit by krzysztof narkowicz
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
// close to the identity for dark colours and rolls bright colours off smoothly towards 1
vec3 toneMap(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

// convert linear rgb to srgb for display (copied)
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
    vec3 color = texture(sceneTexture, fragTexCoord).rgb;
    color += bloomStrength * texture(bloomTexture, fragTexCoord).rgb;
    fragColor = vec4(linearToSrgb(toneMap(color)), 1.0);
}
