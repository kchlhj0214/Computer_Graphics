#version 330 core
layout (location = 0) in vec2 vPosition;
uniform vec2 uOffset;
uniform float uSize;
uniform vec2 uWindowSize;
void main(void)
{
    vec2 pixel = vPosition * uSize + uOffset;
    vec2 ndc = vec2(pixel.x / uWindowSize.x * 2.0 - 1.0,
                    1.0 - pixel.y / uWindowSize.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
}
