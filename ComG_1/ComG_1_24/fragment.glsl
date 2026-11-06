#version 330 compatibility
in vec3 worldPosition;
in vec3 worldNormal;
uniform vec3 uColor;
uniform vec3 eyePosition;
uniform vec2 ceilingSize;
uniform float ceilingHeight;
uniform ivec2 lightSamples;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;
out vec4 FragColor;
void main(void)
{
    vec3 N=normalize(worldNormal);
    vec3 V=normalize(eyePosition-worldPosition);
    vec3 result=vec3(0.0);
    // Equal-area samples approximate a downward-facing rectangular ceiling light.
    for(int z=0;z<lightSamples.y;++z) {
        for(int x=0;x<lightSamples.x;++x) {
            vec2 uv=(vec2(x,z)+vec2(0.5))/vec2(lightSamples);
            vec3 light=vec3((uv.x-0.5)*ceilingSize.x,ceilingHeight,(uv.y-0.5)*ceilingSize.y);
            vec3 L=normalize(light-worldPosition);
            float diffuse=max(dot(N,L),0.0);
            float specular=0.0;
            if(diffuse>0.0)specular=pow(max(dot(reflect(-L,N),V),0.0),shininess);
            float emissionDirection=max(L.y,0.0);
            result+=emissionDirection*(diffuseStrength*diffuse*uColor+specularStrength*specular*vec3(1.0));
        }
    }
    FragColor=vec4(result/float(lightSamples.x*lightSamples.y),1.0);
}
