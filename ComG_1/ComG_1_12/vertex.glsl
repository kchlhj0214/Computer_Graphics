#version 330 core

layout (location = 0) in vec4 vSegment;
uniform vec2 uOffset;
uniform float uSize;
uniform float uAngle;
uniform bool uPath;

void main(void)
{
    vec2 position;
    if (uPath) {
        //--- 각 선분을 두 삼각형으로 만들어 드라이버와 관계없이 두께 3픽셀 유지.
        const vec2 corners[6] = vec2[6](
            vec2(0,-1), vec2(1,-1), vec2(1,1),
            vec2(0,-1), vec2(1,1), vec2(0,1));
        vec2 delta = vSegment.zw - vSegment.xy;
        vec2 normal = vec2(-delta.y, delta.x) / max(length(delta), 0.0001);
        vec2 corner = corners[gl_VertexID];
        position = mix(vSegment.xy, vSegment.zw, corner.x) + normal * corner.y * 1.5;
    }
    else {
        //--- 밑변 1, 높이 2의 이등변삼각형을 생성하고 크기/회전/위치를 적용.
        const vec2 triangle[3] = vec2[3](vec2(0,-1), vec2(-0.5,1), vec2(0.5,1));
        float c = cos(uAngle), s = sin(uAngle);
        position = mat2(c, s, -s, c) * (triangle[gl_VertexID] * uSize) + uOffset;
    }
    gl_Position = vec4(position.x / 800.0 - 1.0, 1.0 - position.y / 600.0, 0.0, 1.0);
}