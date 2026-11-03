#version 410

// this shader is used for drawing the background. it takes the cubemap of the sky we generate in environmentMap
// and the fragDirection which world direction the camera is looking along in this case. it uses the fragdirection
// to sample thecubemap (always at maximum detail so lod in textureLod is 0.0)

// we pretend the sky (sides of the cube ) is infinitely distant, so position inside it doesnt matter.
// we can say we are always at the origin. so when sampling the only thing that matters is the vector from the origin (or wherever)
// to where we are looking. we use that vector (in this case fragDirecton) to sample the cubemap with textureLod.

uniform samplerCube environmentMap;
in vec3 fragDirection;
layout(location = 0) out vec4 fragColor;

void main()
{
    // linear and unclamped so the bright sun in the hdr sky can bloom
    vec3 color = textureLod(environmentMap, normalize(fragDirection), 0.0).rgb;
    fragColor = vec4(color, 1.0);
}
