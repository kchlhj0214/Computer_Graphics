#version 330 core

//--- out_Color: 버텍스 셰이더에서 전달받는 색상값
//--- FragColor: 출력할 색상값으로 프레임버퍼에 전달됨
in vec3 out_Color;
out vec4 FragColor;

void main(void)
{
    FragColor = vec4(out_Color, 1.0);
}
