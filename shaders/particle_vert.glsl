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

    // projection[1][1] scales vertical distances; half the framebuffer height
    // converts NDC distances to pixels. Dividing by depth gives perspective size.
    float depth = max(-viewPosition.z, 0.001);
    float diameterPixels = size * projectionMatrix[1][1] * framebufferHeight * 0.5 / depth;
    gl_PointSize = diameterPixels;
    particleOpacity = opacity;
}
