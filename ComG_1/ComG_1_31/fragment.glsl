#version 330 compatibility
in vec2 TexCoord;
in vec3 skyDirection;
uniform sampler2D outTexture;
uniform samplerCube skyboxTexture;
uniform int drawMode;
uniform bool useTexture;
uniform vec3 uColor;
uniform float objectAlpha;
out vec4 FragColor;

void main()
{
    if (drawMode == 2)
        FragColor = texture(skyboxTexture, skyDirection);
    else if (drawMode == 1)
        FragColor = texture(outTexture, TexCoord);
    else if (useTexture)
        FragColor = vec4(texture(outTexture, TexCoord).rgb, objectAlpha);
    else
        FragColor = vec4(uColor, 1.0);
}
