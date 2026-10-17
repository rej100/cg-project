#version 410

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd; // linear rgb
    vec3 ks;
    float shininess;
    float transparency;
};

// in general everything is linear rgb except for the fragColor output

// can be stored as srgb on gpu but texture(...) will return linear rgb
uniform sampler2D baseColorMap;
uniform bool hasBaseColorTexture;
uniform vec3 directionToLight; // World-space direction from the surface toward the light.
uniform int shadingMode;

in vec3 fragPosition; // World-space position.
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

// this has to be the same as Application::ShadingMode
const int SHADING_LIT = 0;
const int SHADING_BASE_COLOR = 1;
const int SHADING_NORMALS = 2;

// input and output are linear rgb
vec3 shadeDiffuse(vec3 baseColor, vec3 normal)
{
    const float ambient = 0.25;
    const float lightStrength = 0.75;
    float diffuse = max(dot(normal, normalize(directionToLight)), 0.0);
    return baseColor * (ambient + lightStrength * diffuse);
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
    vec3 normal = normalize(fragNormal);
    if (shadingMode == SHADING_NORMALS) {
        // change normal xyz from [-1, 1] to [0, 1] for RGB
        // Diagnostic display RGB; bypass the linear-light material/lighting path.
        fragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }

    vec3 baseColor = hasBaseColorTexture ? texture(baseColorMap, fragTexCoord).rgb : kd;
    vec3 color = shadingMode == SHADING_LIT ? shadeDiffuse(baseColor, normal) : baseColor;

    fragColor = vec4(linearToSrgb(color), 1.0);
}
