#version 330 compatibility
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
out vec2 TexCoord;

void main()
{
    // 좌표축 원뿔의 GLU 정점과 VAO 위치 속성 0을 같은 방식으로 받습니다.
    gl_Position = projectionTransform * viewTransform * modelTransform * gl_Vertex;
    TexCoord = vTexCoord;
}
