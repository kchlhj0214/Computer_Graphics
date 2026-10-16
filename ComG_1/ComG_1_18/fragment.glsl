#version 330 compatibility
in vec3 worldPosition;
in vec3 worldNormal;
uniform vec3 uColor;
uniform bool useShading;
uniform vec3 lightPosition;
uniform vec3 eyePosition;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;
out vec4 FragColor;
void main(void)
{
    vec3 color = uColor;
    if (useShading) {
        vec3 N = normalize(worldNormal);
        vec3 L = normalize(lightPosition - worldPosition);
        vec3 V = normalize(eyePosition - worldPosition);
        float diffuse = max(dot(N, L), 0.0);
        float specular = 0.0;
        if (diffuse > 0.0)
            specular = pow(max(dot(reflect(-L, N), V), 0.0), shininess);
        color = diffuseStrength * diffuse * uColor + specularStrength * specular * vec3(1.0);
    }
    FragColor = vec4(color, 1.0);
}
