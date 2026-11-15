#version 330 compatibility
layout(location = 1) in vec3 vNormal;
uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
out vec3 worldPosition;
out vec3 worldNormal;
void main(void)
{
    // VBO와 GLU 정점을 같은 셰이더에서 처리합니다.
    vec4 position = modelTransform * gl_Vertex;
    worldPosition = position.xyz;
    worldNormal = transpose(inverse(mat3(modelTransform))) * vNormal;
    gl_Position = projectionTransform * viewTransform * position;
}
