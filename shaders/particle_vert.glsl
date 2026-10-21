#version 410

layout(location = 0) in vec3 position;
layout(location = 1) in float size; // world-space diameter in metres
layout(location = 2) in float opacity;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform float framebufferHeight;

out float particleOpacity;

void main()
{
    vec4 viewPosition = viewMatrix * vec4(position, 1.0);
    gl_Position = projectionMatrix * viewPosition;

    // size is in meters and we need to convert it to pixels
    // how far away from the camera is the particle
    float depth = max(-viewPosition.z, 0.001);

    // convert from meters to ndc and scale by depth (smaller when further away)
    float diameterNDC = size * projectionMatrix[1][1] / depth;
    // convert from ndc to pixels
    float diameterPixels = diameterNDC * framebufferHeight * 0.5;

    gl_PointSize = diameterPixels;
    particleOpacity = opacity;
}
