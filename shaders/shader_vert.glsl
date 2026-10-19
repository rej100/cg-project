#version 410

uniform mat4 modelMatrix;
uniform mat4 viewProjectionMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
// xyz: object-space tangent direction (along increasing U)
// w: bitangent sign.
layout(location = 3) in vec4 tangentAndBitangentSign;

out vec3 fragPosition; // World-space position.
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec4 fragTangentAndBitangentSign;

void main()
{
    vec4 worldPosition = modelMatrix * vec4(position, 1.0);
    fragPosition = worldPosition.xyz;
    gl_Position = viewProjectionMatrix * worldPosition;
    
    fragNormal      = normalModelMatrix * normal;
    fragTexCoord    = texCoord;

    fragTangentAndBitangentSign = vec4(mat3(modelMatrix) * tangentAndBitangentSign.xyz, tangentAndBitangentSign.w);
}
