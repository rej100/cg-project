#version 410

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd; // linear rgb
    vec3 ks;
    float shininess;
    float transparency;
    vec3 emission; // linear rgb
};

// in general everything is linear rgb except for the fragColor output

// can be stored as srgb on gpu but texture(...) will return linear rgb
uniform sampler2D baseColorMap;
uniform bool hasBaseColorTexture;
uniform int shadingMode;
uniform bool enableEmission;
uniform float ambientStrength;

struct DirectionalLight {
    vec3 directionToLight; // World-space direction toward the light.
    vec3 color; // linear rgb
    float intensity;
    bool enabled;
};

struct SpotLight {
    vec3 position;
    vec3 direction; // World-space direction from the light toward its target.
    vec3 color; // linear rgb
    float intensity;
    float innerConeCos;
    float outerConeCos;
    bool enabled;
};

uniform DirectionalLight sun;
uniform SpotLight spotlight;

in vec3 fragPosition; // World-space position.
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

// this has to be the same as Application::ShadingMode
const int SHADING_LIT = 0;
const int SHADING_BASE_COLOR = 1;
const int SHADING_NORMALS = 2;

// input and output are linear rgb
vec3 shadeDiffuse(vec3 baseColor, vec3 normal, vec3 directionToLight, vec3 incomingLight)
{
    float diffuse = max(dot(normal, directionToLight), 0.0);
    return baseColor * incomingLight * diffuse;
}

vec3 shadeSun(vec3 baseColor, vec3 normal)
{
    if (!sun.enabled)
        return vec3(0.0);

    return shadeDiffuse(baseColor, normal, normalize(sun.directionToLight), sun.color * sun.intensity);
}

vec3 shadeSpotlight(vec3 baseColor, vec3 normal)
{
    if (!spotlight.enabled)
        return vec3(0.0);

    vec3 toLight = spotlight.position - fragPosition;
    float distanceSquared = dot(toLight, toLight);
    if (distanceSquared < 0.000001)
        return vec3(0.0);

    vec3 directionToLight = toLight / sqrt(distanceSquared);
    float coneAlignment = dot(spotlight.direction, -directionToLight);
    float coneFactor = smoothstep(spotlight.outerConeCos, spotlight.innerConeCos, coneAlignment);
    // Clamp inverse-square attenuation within 0.1 m of the source.
    float attenuation = 1.0 / max(distanceSquared, 0.01);
    vec3 incomingLight = spotlight.color * spotlight.intensity * attenuation * coneFactor;

    return shadeDiffuse(baseColor, normal, directionToLight, incomingLight);
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
    vec3 color = baseColor;
    if (shadingMode == SHADING_LIT) {
        color = baseColor * ambientStrength;
        color += shadeSun(baseColor, normal);
        color += shadeSpotlight(baseColor, normal);
        if (enableEmission)
            color += emission;
    }

    fragColor = vec4(linearToSrgb(color), 1.0);
}
