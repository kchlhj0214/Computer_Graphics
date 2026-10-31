#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
out vec3 worldPosition;
out vec3 worldNormal;
void main(void)
{
    vec4 world=modelTransform*vec4(position,1.0);
    worldPosition=world.xyz;
    worldNormal=transpose(inverse(mat3(modelTransform)))*normal;
    gl_Position=projectionTransform*viewTransform*world;
}
