#version 330 compatibility
uniform vec3 uColor;
uniform bool uSphere;
in vec3 normal;
out vec4 FragColor;
void main(void)
{
    float light = 1.0;
    if (uSphere)
        light = 0.35 + 0.65 * max(dot(normalize(normal), normalize(vec3(-0.3, 0.6, 1.0))), 0.0);
    FragColor = vec4(uColor * light, 1.0);
}
