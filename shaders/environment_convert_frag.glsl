#version 410

uniform sampler2D panoramaMap;
in vec3 fragDirection;
layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

void main()
{
    vec3 direction = normalize(fragDirection);
    // Longitude covers the full width; latitude covers the full height.
    // The upload flipped the rows, so v=1 is the top (+Y) of the panorama.
    vec2 uv;
    uv.x = atan(direction.z, direction.x) / (2.0 * PI) + 0.5;
    uv.y = asin(clamp(direction.y, -1.0, 1.0)) / PI + 0.5;
    fragColor = vec4(texture(panoramaMap, uv).rgb, 1.0);
}
