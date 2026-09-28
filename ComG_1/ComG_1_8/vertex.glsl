#version 330 core

uniform vec2 uOffset;        //--- 클릭 위치 (픽셀)
uniform float uSize;         //--- 생성 시 밑변 길이 (50~150)
uniform float uAnimationTime; //--- 우클릭으로 활성화된 누적 시간

void main(void)
{
    vec2 position;
    if (gl_VertexID < 3) {
        //--- 삼각형의 꼭짓점과 확대/축소를 셰이더에서 계산한다.
        const vec2 triangle[3] = vec2[3](
            vec2(0.0, -1.0), vec2(-0.5, 1.0), vec2(0.5, 1.0));
        //--- 초당 100픽셀, 밑변 20~200을 왕복하며 높이는 밑변의 2배.
        float phase = mod(uSize - 20.0 + uAnimationTime * 100.0, 360.0);
        float size = 20.0 + 180.0 - abs(phase - 180.0);
        position = triangle[gl_VertexID] * size + uOffset;
    }
    else {
        const vec2 axes[4] = vec2[4](
            vec2(0.0, 600.0), vec2(1200.0, 600.0),
            vec2(600.0, 0.0), vec2(600.0, 1200.0));
        position = axes[gl_VertexID - 3];
    }
    gl_Position = vec4(position.x / 600.0 - 1.0,
                       1.0 - position.y / 600.0, 0.0, 1.0);
}