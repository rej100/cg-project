#version 410

layout(location = 0) in vec3 position;
uniform mat4 viewProjectionMatrix;

out vec3 fragDirection;

void main()
{
    // A cube centered at the eye supplies one direction per background pixel.
    // The caller removes view translation but keeps rotation and projection.
    fragDirection = position;
    gl_Position = viewProjectionMatrix * vec4(position, 1.0);
}
