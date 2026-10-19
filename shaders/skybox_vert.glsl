#version 410

// this shader takes a vertex from our cube and the view projection matrix and passes the correct gl_position.
// we dont need the model matrix since we only want the cube at the origin
// it also passes the fragDirection which is just the position of the vertex. because the cube is centered at the origin,
// every one of its vertex positions also descrbes a vector from the center to that vertex. since the cube is just at the origin
// its axes are aligned with the world axes so this can also just be used for getting the world space direction from the origin to
// the vertex. this is needed so we know which point of the sky texture to sample for the frag

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
