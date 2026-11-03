#version 410

// one direction of a 9 tap gaussian blur. running it horizontally and then vertically
// gives the same result as a 2d gaussian but with 9 + 9 samples instead of 9 * 9

uniform sampler2D inputTexture;
uniform bool horizontal;

in vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

// weights for the center and 4 neighbours on each side, they add up to 1
const float WEIGHTS[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    // the distance to the next texel in the blur direction
    vec2 texelSize = 1.0 / vec2(textureSize(inputTexture, 0));
    vec2 blurStep = horizontal ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);

    vec3 color = texture(inputTexture, fragTexCoord).rgb * WEIGHTS[0];
    for (int i = 1; i < 5; ++i) {
        color += texture(inputTexture, fragTexCoord + blurStep * float(i)).rgb * WEIGHTS[i];
        color += texture(inputTexture, fragTexCoord - blurStep * float(i)).rgb * WEIGHTS[i];
    }
    fragColor = vec4(color, 1.0);
}
