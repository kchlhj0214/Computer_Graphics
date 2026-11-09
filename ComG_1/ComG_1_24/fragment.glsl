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
uniform vec3 cargoLights[3];
uniform float cargoLightStrength;
uniform float cargoLightAttenuation;
uniform mat4 worldToTruck;
uniform vec3 cargoLightMin;
uniform vec3 cargoLightMax;
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
    result/=float(count.x*count.y);
    // 짐칸 내부에만 점조명을 적용한다. 바깥 바닥·벨트·차체에는 천장 조명만 적용된다.
    vec3 truckPosition=(worldToTruck*vec4(worldPosition,1.0)).xyz;
    bool insideCargo=all(greaterThanEqual(truckPosition,cargoLightMin))
        && all(lessThanEqual(truckPosition,cargoLightMax));
    for(int i=0;i<3 && insideCargo;++i) {
        vec3 delta=cargoLights[i]-worldPosition;
        float distanceSquared=max(dot(delta,delta),0.0001);
        vec3 L=delta*inversesqrt(distanceSquared);
        float diffuse=max(dot(N,L),0.0);
        float specular=diffuse>0.0 ? pow(max(dot(reflect(-L,N),V),0.0),shininess) : 0.0;
        result+=cargoLightStrength/(1.0+cargoLightAttenuation*distanceSquared)
            *(diffuse*uColor+specularStrength*specular*vec3(1.0));
    }
    FragColor=vec4(result,1.0);
}
