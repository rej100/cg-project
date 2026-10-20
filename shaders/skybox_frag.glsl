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
    vec3 color = textureLod(environmentMap, normalize(fragDirection), 0.0).rgb;
    fragColor = vec4(linearToSrgb(color), 1.0);
}
