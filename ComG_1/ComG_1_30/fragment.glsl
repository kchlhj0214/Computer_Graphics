#version 330 compatibility
in vec2 TexCoord;
uniform sampler2D outTexture;
uniform bool useTexture;
uniform vec3 uColor;
out vec4 FragColor;

void main()
{
    if (useTexture)
        FragColor = texture(outTexture, TexCoord);
    else
        FragColor = vec4(uColor, 1.0);
}
