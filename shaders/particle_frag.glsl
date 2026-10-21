#version 410

in float particleOpacity;
layout(location = 0) out vec4 fragColor;

void main()
{
    // gl_PointCoord runs from 0 to 1 across the point's square. Map it to a
    // unit disk and fade from its centre to zero at the circular edge.
    vec2 offset = gl_PointCoord * 2.0 - 1.0;
    float radialOpacity = max(1.0 - dot(offset, offset), 0.0);
    float opacity = radialOpacity * particleOpacity;

    // pure cyan has the same RGB values in linear and sRGB. The current SDR
    // target adds these display values; linear-light blending comes with HDR.
    fragColor = vec4(0.0, 1.0, 1.0, opacity);
}
