#version 330 core

uniform vec3 uColor; //--- 삼각형의 면 색상 또는 테두리/축의 검은색
out vec4 FragColor;

void main(void)
{
    FragColor = vec4(uColor, 1.0);
}