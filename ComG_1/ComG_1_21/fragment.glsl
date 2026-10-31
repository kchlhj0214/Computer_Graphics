#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
uniform vec3 uColor;
uniform vec3 eyePosition;
uniform vec3 lightPosition;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;
out vec4 FragColor;
void main(void)
{
    vec3 N=normalize(worldNormal);
    vec3 V=normalize(eyePosition-worldPosition);
    // 카메라 쪽 점광원의 확산광과 퐁 반사광만 사용한다.
    vec3 L=normalize(lightPosition-worldPosition);
    float diffuse=max(dot(N,L),0.0);
    float specular=0.0;
    if(diffuse>0.0)specular=pow(max(dot(reflect(-L,N),V),0.0),shininess);
    vec3 result=diffuseStrength*diffuse*uColor+specularStrength*specular*vec3(1.0);
    FragColor=vec4(result,1.0);
}
