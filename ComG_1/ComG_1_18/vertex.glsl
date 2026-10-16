#version 330 compatibility
uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
out vec3 worldPosition;
out vec3 worldNormal;
void main(void)
{
    vec4 world = modelTransform * gl_Vertex;
    worldPosition = world.xyz;
    worldNormal = transpose(inverse(mat3(modelTransform))) * gl_Normal;
    gl_Position = projectionTransform * viewTransform * world;
}
