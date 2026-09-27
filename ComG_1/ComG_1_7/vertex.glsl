#version 330 core

//--- Position: attribute index 0
//--- Color: attribute index 1
layout (location = 0) in vec3 vPosition; //--- 위치 변수: attribute position 0
layout (location = 1) in vec3 vColor;    //--- 컬러 변수: attribute position 1

uniform vec2 uOffset; //--- C++에서 전달받는 도형의 중심 위치

out vec3 out_Color; //--- 프래그먼트 셰이더에게 전달

void main(void)
{
    vec3 movedPosition = vPosition;
    movedPosition.xy += uOffset;
    gl_Position = vec4(movedPosition, 1.0);
    out_Color = vColor;
}
