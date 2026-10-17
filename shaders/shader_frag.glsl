#version 410

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};

uniform sampler2D baseColorMap;
uniform bool hasBaseColorTexture;
uniform vec3 directionToLight; // World-space direction from the surface toward the light.
uniform int shadingMode;

in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

// Keep these values in sync with Application::ShadingMode.
const int SHADING_LIT = 0;
const int SHADING_BASE_COLOR = 1;
const int SHADING_NORMALS = 2;

vec3 shadeDiffuse(vec3 baseColor, vec3 normal)
{
    const float ambient = 0.25;
    const float lightStrength = 0.75;
    float diffuse = max(dot(normal, normalize(directionToLight)), 0.0);
    return baseColor * (ambient + lightStrength * diffuse);
}

vec3 linearToSrgb(vec3 color)
{
    color = max(color, vec3(0.0));
    vec3 low = 12.92 * color;
    vec3 high = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, lessThanEqual(color, vec3(0.0031308)));
}

void main()
{
    vec3 normal = normalize(fragNormal);
    if (shadingMode == SHADING_NORMALS) {
        // Remap signed XYZ directions from [-1, 1] into display RGB [0, 1].
        fragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }

    // sRGB textures are decoded by OpenGL; constant material colors are already linear.
    vec3 baseColor = hasBaseColorTexture
        ? texture(baseColorMap, fragTexCoord).rgb
        : kd;
    vec3 color = shadingMode == SHADING_LIT ? shadeDiffuse(baseColor, normal) : baseColor;
    fragColor = vec4(linearToSrgb(color), 1.0);
}
