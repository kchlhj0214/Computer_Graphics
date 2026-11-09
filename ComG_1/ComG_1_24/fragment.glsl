#version 330 compatibility
in vec3 worldPosition;
in vec3 worldNormal;
uniform vec3 uColor;
uniform vec3 eyePosition;
uniform vec2 ceilingSize;
uniform float ceilingHeight;
uniform ivec2 lightSamples;
uniform int localSamples;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;
out vec4 FragColor;
void main(void)
{
    vec3 N=normalize(worldNormal);
    vec3 V=normalize(eyePosition-worldPosition);
    vec3 result=vec3(0.0);
    // 맵 전체 격자 중 현재 지점과 가까운 조명만 계산한다.
    ivec2 count=min(lightSamples,ivec2(localSamples));
    ivec2 cell=ivec2(floor((worldPosition.xz/ceilingSize+0.5)*vec2(lightSamples)));
    ivec2 first=clamp(cell-count/2,ivec2(0),lightSamples-count);
    for(int z=0;z<count.y;++z) {
        for(int x=0;x<count.x;++x) {
            vec2 uv=(vec2(first+ivec2(x,z))+vec2(0.5))/vec2(lightSamples);
            vec3 light=vec3((uv.x-0.5)*ceilingSize.x,ceilingHeight,(uv.y-0.5)*ceilingSize.y);
            vec3 L=normalize(light-worldPosition);
            float diffuse=max(dot(N,L),0.0);
            float specular=0.0;
            if(diffuse>0.0)specular=pow(max(dot(reflect(-L,N),V),0.0),shininess);
            float emissionDirection=max(L.y,0.0);
            result+=emissionDirection*(diffuseStrength*diffuse*uColor+specularStrength*specular*vec3(1.0));
        }
    }
    FragColor=vec4(result/float(count.x*count.y),1.0);
}
