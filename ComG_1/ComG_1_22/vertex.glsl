#version 330 compatibility
layout(location = 0) in vec3 position;
uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
void main(void)
{
    gl_Position = projectionTransform * viewTransform * modelTransform * vec4(position, 1.0);
}
