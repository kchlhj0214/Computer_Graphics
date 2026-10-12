#version 330 compatibility
in vec3 out_Color;
in vec3 out_Normal;
uniform bool useShading;
uniform vec3 uColor;
out vec4 FragColor;
void main(void)
{
    vec3 color = out_Color;
    if (useShading) {
        // 저장된 법선에 환경광과 확산광을 적용하여 하늘색 표면의 굴곡을 표현한다.
        vec3 normal = normalize(out_Normal);
        vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.8));
        float diffuse = max(dot(normal, lightDirection), 0.0);
        color = uColor * (0.30 + 0.70 * diffuse);
    }
    FragColor = vec4(color, 1.0);
}
