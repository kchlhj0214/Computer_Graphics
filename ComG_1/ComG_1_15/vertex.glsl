#version 330 compatibility
// GLU는 gl_Vertex로 정점을 전달하고, VBO는 이에 대응하는 속성 0을 사용한다.
uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
layout (location = 1) in vec3 vColor;
uniform vec3 uColor;
uniform bool useVertexColor;
out vec3 out_Color;
void main(void)
{
    gl_Position = projectionTransform * viewTransform * modelTransform * gl_Vertex;
    out_Color = useVertexColor ? vColor : uColor;
}
