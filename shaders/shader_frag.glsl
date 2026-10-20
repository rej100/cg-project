#version 410

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd; // linear rgb
    vec3 ks;
    float shininess;
    float transparency;
    float metallic;
    float roughness;
    vec3 emission; // linear rgb
    float normalMapStrength;
};

// in general everything is linear rgb except for the fragColor output

// can be stored as srgb on gpu but texture(...) will return linear rgb
uniform sampler2D baseColorMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;
uniform samplerCube environmentMap;
uniform bool hasBaseColorTexture;
uniform bool hasRoughnessTexture;
uniform bool hasNormalTexture;
uniform int shadingMode;
uniform bool enableEmission;
uniform bool enableMaterialTextures;
uniform bool enableNormalMapping;
uniform bool enableEnvironmentReflections;
uniform float environmentMaxLOD;
uniform float ambientStrength;

uniform vec3 cameraPosition;

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
    float coneCos;
    bool enabled;
};

uniform DirectionalLight sun;
uniform SpotLight spotlight;

in vec3 fragPosition; // World-space position.
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragTangentAndBitangentSign; // xyz: interpolated tangent w: bitangent sign.

layout(location = 0) out vec4 fragColor;

// this has to be the same as Application::ShadingMode
const int SHADING_LIT = 0;
const int SHADING_PBR = 1;
const int SHADING_BASE_COLOR = 2;
const int SHADING_NORMALS = 3;

const float PI = 3.14159265359;

// for when roughness is 0 so the pbr doesnt break
const float MIN_ROUGHNESS = 0.045;

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
    if (dot(toLight, toLight) < 0.000001)
        return vec3(0.0);

    vec3 directionToLight = normalize(toLight);
    float coneAlignment = dot(spotlight.direction, -directionToLight);
    if (coneAlignment < spotlight.coneCos)
        return vec3(0.0);

    return shadeDiffuse(baseColor, normal, directionToLight, spotlight.color * spotlight.intensity);
}

// cook-torrance with ggx distribution, smith visibility, and schlick fresnel
// https://learnopengl.com/PBR/Theory
// https://learnopengl.com/PBR/Lighting
float pbrDistribution(float nDotH, float alpha)
{
    float alpha2 = alpha * alpha;
    float denominator = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denominator * denominator);
}

float pbrVisibility(float nDotV, float nDotL, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return (nDotV / (nDotV * (1.0 - k) + k)) * (nDotL / (nDotL * (1.0 - k) + k));
}

vec3 pbrFresnel(float hDotV, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - hDotV, 5.0);
}

vec3 shadePBR(vec3 baseColor, float metallic, float roughness, vec3 normal, vec3 viewDirection, vec3 directionToLight, vec3 incomingLight)
{
    vec3 halfVector = normalize(viewDirection + directionToLight);
    float nDotL = max(dot(normal, directionToLight), 0.0);
    float nDotV = max(dot(normal, viewDirection), 0.0);
    float nDotH = max(dot(normal, halfVector), 0.0);
    float hDotV = max(dot(halfVector, viewDirection), 0.0);

    // reflect 4 percent at all angles for dielectricts, base color for metals
    vec3 f0 = mix(vec3(0.04), baseColor, metallic);
    float alpha = roughness * roughness;

    float distribution = pbrDistribution(nDotH, alpha);
    float visibility = pbrVisibility(nDotV, nDotL, roughness);
    vec3 fresnel = pbrFresnel(hDotV, f0);

    vec3 specular = distribution * visibility * fresnel / max(4.0 * nDotV * nDotL, 0.0001);
    vec3 diffuse = (1.0 - fresnel) * (1.0 - metallic) * baseColor / PI;

    return (diffuse + specular) * incomingLight * nDotL;
}

vec3 shadePbrSun(vec3 baseColor, float metallic, float roughness, vec3 normal, vec3 viewDirection)
{
    if (!sun.enabled)
        return vec3(0.0);

    return shadePBR(baseColor, metallic, roughness, normal, viewDirection, normalize(sun.directionToLight), sun.color * sun.intensity);
}

vec3 shadePbrSpotlight(vec3 baseColor, float metallic, float roughness, vec3 normal, vec3 viewDirection)
{
    if (!spotlight.enabled)
        return vec3(0.0);

    vec3 toLight = spotlight.position - fragPosition;
    if (dot(toLight, toLight) < 0.000001)
        return vec3(0.0);

    vec3 directionToLight = normalize(toLight);
    float coneAlignment = dot(spotlight.direction, -directionToLight);
    if (coneAlignment < spotlight.coneCos)
        return vec3(0.0);

    return shadePBR(baseColor, metallic, roughness, normal, viewDirection, directionToLight, spotlight.color * spotlight.intensity);
}

vec3 shadeEnvironment(vec3 baseColor, float metallic, float roughness, vec3 normal, vec3 viewDirection)
{
    if (!enableEnvironmentReflections)
        return vec3(0.0);

    // to get the color of the skybox that we are reflecting we go camera to our point, reflect about our normal,
    // check where that lands on the skybox
    vec3 reflectedDirection = reflect(-viewDirection, normal);
    float lod = clamp(roughness, 0.0, 1.0) * environmentMaxLOD;
    vec3 reflectedColor = textureLod(environmentMap, reflectedDirection, lod).rgb;

    // use fresnel for how much we are reflecting 
    vec3 f0 = mix(vec3(0.04), baseColor, metallic);
    float nDotV = clamp(dot(normal, viewDirection), 0.0, 1.0);
    return pbrFresnel(nDotV, f0) * reflectedColor;
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

// get the normal from the normal map is available
vec3 resolveShadingNormal(vec3 meshNormal)
{
    if (!enableNormalMapping || !hasNormalTexture)
        return meshNormal;


    vec3 tangent = fragTangentAndBitangentSign.xyz;

    // make sure the tangent is perpendicular to the frag normal
    tangent -= meshNormal * dot(meshNormal, tangent);

    // if bad tangent just use the normal
    if (dot(tangent, tangent) < 0.000001)
        return meshNormal;

    tangent = normalize(tangent);
    vec3 bitangent = fragTangentAndBitangentSign.w * cross(meshNormal, tangent);
    mat3 tangentToWorld = mat3(tangent, bitangent, meshNormal);

    // normal map values are 0 to 1 but we want -1 to 1
    vec3 mappedNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;

    // scale by the normal map strenght
    mappedNormal.xy *= normalMapStrength;
    return normalize(tangentToWorld * mappedNormal);
}

void main()
{
    vec3 normal = resolveShadingNormal(normalize(fragNormal));
    if (shadingMode == SHADING_NORMALS) {
        // change normal xyz from [-1, 1] to [0, 1] for RGB
        fragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }

    vec3 baseColor = kd;
    float surfaceRoughness = max(roughness, MIN_ROUGHNESS);
    if (enableMaterialTextures) {
        if (hasBaseColorTexture)
            baseColor = texture(baseColorMap, fragTexCoord).rgb;
        if (hasRoughnessTexture)
            surfaceRoughness = max(texture(roughnessMap, fragTexCoord).r, MIN_ROUGHNESS);
    }

    vec3 color = baseColor;
    if (shadingMode == SHADING_LIT) {
        color = baseColor * ambientStrength;
        color += shadeSun(baseColor, normal);
        color += shadeSpotlight(baseColor, normal);
        if (enableEmission)
            color += emission;
    } else if (shadingMode == SHADING_PBR) {
        vec3 viewDirection = normalize(cameraPosition - fragPosition);
        color = baseColor * ambientStrength;
        color += shadePbrSun(baseColor, metallic, surfaceRoughness, normal, viewDirection);
        color += shadePbrSpotlight(baseColor, metallic, surfaceRoughness, normal, viewDirection);
        color += shadeEnvironment(baseColor, metallic, surfaceRoughness, normal, viewDirection);
        if (enableEmission)
            color += emission;
    }

    fragColor = vec4(linearToSrgb(color), 1.0);
}
