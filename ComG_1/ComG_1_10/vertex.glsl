#version 330 core

layout (location = 0) in vec2 vPosition;

uniform vec2 uOffset;
uniform float uAngle;
uniform vec2 uWindowSize;

void main(void)
{
    float c = cos(uAngle);
    float s = sin(uAngle);
    vec2 rotated = vec2(
        c * vPosition.x - s * vPosition.y,
        s * vPosition.x + c * vPosition.y);
    vec2 pixel = rotated + uOffset;
    vec2 ndc = vec2(
        pixel.x / uWindowSize.x * 2.0 - 1.0,
        1.0 - pixel.y / uWindowSize.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
}
