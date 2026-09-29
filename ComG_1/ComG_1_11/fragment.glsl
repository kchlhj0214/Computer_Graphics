#version 330 core
uniform vec3 uColor;
uniform float uAlpha;
out vec4 FragColor;
void main(void)
{
    FragColor = vec4(uColor, uAlpha);
}
