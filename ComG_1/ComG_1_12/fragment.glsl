#version 330 core

uniform vec3 uColor;
out vec4 FragColor;

void main(void)
{
    FragColor = vec4(uColor, 1.0);
}
