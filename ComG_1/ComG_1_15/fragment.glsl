#version 330 compatibility
in vec3 out_Color;
out vec4 FragColor;
void main(void)
{
    FragColor = vec4(out_Color, 1.0);
}
