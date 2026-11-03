#version 410

// one triangle that covers the whole screen, no vertex buffer needed.
// vertex 0, 1, 2 become (-1, -1), (3, -1), (-1, 3) and the part outside the screen is clipped away

out vec2 fragTexCoord;

void main()
{
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2) * 2.0 - 1.0;
    // positions are -1 to 1 but texture coordinates are 0 to 1
    fragTexCoord = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
