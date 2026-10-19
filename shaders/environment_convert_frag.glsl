#version 410

// this shader is used to draw the faces of the cubemap. it gets the sky texture panoramaMap and the fragdirection
// the fragdirection tells us which world direction this output pixel is for
// this shader converts from the direction, to u v coordinates on the texture so we can sample it and get the color of the pixel
// of the cubemap


uniform sampler2D panoramaMap;
in vec3 fragDirection;
layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

void main()
{

    vec3 direction = normalize(fragDirection);

    // the upload flips the rows so v=1 is the top (+y) of the panorama
    vec2 uv;

    // https://learnopengl.com/PBR/IBL/Diffuse-irradiance
    uv.x = atan(direction.z, direction.x) / (2.0 * PI) + 0.5;
    uv.y = asin(clamp(direction.y, -1.0, 1.0)) / PI + 0.5;
    fragColor = vec4(texture(panoramaMap, uv).rgb, 1.0);
}
