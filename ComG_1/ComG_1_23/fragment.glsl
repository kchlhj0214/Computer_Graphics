#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
in float worldHeight;
uniform float coverCutoff;
void main(void)
{
    if (worldHeight < coverCutoff) discard;
    FragColor = vec4(uColor, 1.0);
}
