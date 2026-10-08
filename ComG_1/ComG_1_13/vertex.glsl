#version 330 compatibility
// GLU supplies gl_Vertex; the axis VBO uses its compatible attribute 0.
uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
out vec3 normal;
void main(void)
{
    gl_Position = projectionTransform * viewTransform * modelTransform * gl_Vertex;
    normal = mat3(viewTransform * modelTransform) * gl_Normal;
}
