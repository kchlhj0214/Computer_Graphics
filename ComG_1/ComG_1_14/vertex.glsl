#version 330 compatibility
// GLU supplies gl_Vertex; the axis VBO uses its compatible attribute 0.
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
