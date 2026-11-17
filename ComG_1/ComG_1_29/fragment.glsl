#version 330 compatibility
in vec3 worldPosition;
in vec3 worldNormal;
uniform vec3 uColor;
uniform vec3 lightPosition;
uniform vec3 eyePosition;
uniform bool useLighting;
uniform float lightIntensity;
uniform bool useTexture;
uniform sampler2D surfaceTexture;
in vec2 textureUV;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;
uniform vec3 fillLightPosition;
uniform float fillLightIntensity;
out vec4 FragColor;
// 각 점광원에서 오는 빛의 반사량을 독립적으로 계산합니다.
vec3 PointLight(vec3 position, vec3 radiance, vec3 N, vec3 V, vec3 surfaceColor)
{
    vec3 L = normalize(position - worldPosition);
    float diffuse = max(dot(N, L), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(reflect(-L, N), V), 0.0), shininess) : 0.0;
    return (diffuseStrength * diffuse * surfaceColor + specularStrength * specular) * radiance;
}
void main(void)
{
    vec4 surface = vec4(uColor,1.0);
    if (useTexture) surface *= texture(surfaceTexture,textureUV);
    if (surface.a < 0.1) discard;
    if (!useLighting) {
        FragColor = vec4(surface.rgb, 1.0);
        return;
    }
    vec3 N = normalize(worldNormal);
    vec3 V = normalize(eyePosition - worldPosition);
    // 주 광원을 끄면 입사광 세기만 0이 됩니다.
    float mainIntensity = lightIntensity;
    vec3 mainColor = vec3(1.0);
    vec3 color = PointLight(lightPosition, mainColor * mainIntensity, N, V, surface.rgb);
    // 흰색 보조 광원은 주 광원의 상태와 무관하게 항상 켜져 있습니다.
    color += PointLight(fillLightPosition, vec3(fillLightIntensity), N, V, surface.rgb);
    FragColor = vec4(color, 1.0);
}
