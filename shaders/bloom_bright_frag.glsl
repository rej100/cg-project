#version 410

// keep only the pixels that are brighter than 1, everything else becomes black

uniform sampler2D sceneTexture;

in vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 color = texture(sceneTexture, fragTexCoord).rgb;
    // perceived brightness, green counts the most (rec. 709 luminance weights)
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        fragColor = vec4(color, 1.0);
    else
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
