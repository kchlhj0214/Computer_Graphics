#version 330 compatibility
in vec3 worldPosition;
in vec3 worldNormal;
uniform vec3 uColor;
uniform vec3 lightPosition;
uniform vec3 eyePosition;
uniform bool useLighting;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;
out vec4 FragColor;
void main(void)
{
    if (!useLighting) {
        FragColor = vec4(uColor, 1.0);
        return;
    }
    // 월드 좌표에서 점광원의 퐁 디퓨스와 스펙큘러만 계산합니다.
    vec3 N = normalize(worldNormal);
    vec3 L = normalize(lightPosition - worldPosition);
    vec3 V = normalize(eyePosition - worldPosition);
    float diffuse = max(dot(N, L), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(reflect(-L, N), V), 0.0), shininess) : 0.0;
    FragColor = vec4(diffuseStrength * diffuse * uColor + specularStrength * specular * vec3(1.0), 1.0);
}
