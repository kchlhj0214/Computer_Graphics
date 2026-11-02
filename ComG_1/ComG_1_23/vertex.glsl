#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
out float worldHeight;
void main(void)
{
    worldHeight = (modelTransform * vec4(position, 1.0)).y;
    gl_Position = projectionTransform * viewTransform * modelTransform * vec4(position, 1.0);
}
