#version 410

// keep only the pixels that are brighter than 1, everything else becomes black

uniform sampler2D sceneTexture;

in vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

const float MAX_BLOOM_BRIGHTNESS = 20.0;

x = 

void main()
{
    vec3 color = texture(sceneTexture, fragTexCoord).rgb;
    // perceived brightness from rec 709 luminance weights
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        // limit only the bloom
        if (brightness > MAX_BLOOM_BRIGHTNESS)
            color *= MAX_BLOOM_BRIGHTNESS / brightness;
        fragColor = vec4(color, 1.0);
    } else {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
