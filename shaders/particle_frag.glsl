#version 410

in float particleOpacity;
layout(location = 0) out vec4 fragColor;

void main()
{
    // gl_PointCord is the position of our frag on the particle square and x and y are from 0 to 1
    // so 0.5 and 0.5 is the center, but we want 0, 0 to be the center so we can calculate the opacity 
    vec2 offset = gl_PointCoord * 2.0 - 1.0;
    // 1 - squared distance from the center
    float radialOpacity = max(1.0 - dot(offset, offset), 0.0);
    float opacity = radialOpacity * particleOpacity;

    fragColor = vec4(0.0, 1.0, 1.0, opacity);
}
