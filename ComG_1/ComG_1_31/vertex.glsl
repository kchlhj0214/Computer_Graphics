#version 330 compatibility
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
uniform int drawMode;
uniform vec2 planeUVScale;
out vec2 TexCoord;
out vec3 skyDirection;

void main()
{
    TexCoord = vTexCoord;
    skyDirection = gl_Vertex.xyz;
    if (drawMode == 1) {
        gl_Position = gl_Vertex;
        TexCoord = (TexCoord - 0.5) * planeUVScale + 0.5;
    } else if (drawMode == 2) {
        // 큐브 배경에는 카메라의 회전만 적용합니다.
        vec4 clip = projectionTransform * mat4(mat3(viewTransform)) * gl_Vertex;
        gl_Position = clip.xyww;
    } else {
        gl_Position = projectionTransform * viewTransform * modelTransform * gl_Vertex;
    }
}
