#version 330 compatibility
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 localGroundPosition;
uniform vec4 uColor;
uniform int uSurface;
out vec4 FragColor;

void main()
{
    vec3 color = uColor.rgb;
    if (uSurface == 1) {
        vec3 lightDirection = normalize(vec3(-0.4, 0.9, 0.6));
        float diffuse = max(dot(normalize(worldNormal), lightDirection), 0.0);
        color *= 0.62 + 0.38 * diffuse;
    }
    else if (uSurface == 2) {
        vec2 distanceToLine = abs(fract(worldPosition.xz + 0.5) - 0.5);
        vec2 feather = fwidth(worldPosition.xz);
        vec2 lines = 1.0 - smoothstep(vec2(0.012), vec2(0.012) + feather, distanceToLine);
        float grid = max(lines.x, lines.y) * (1.0 - smoothstep(0.03, 0.16, max(feather.x, feather.y)));
        color = mix(color, vec3(0.46, 0.53, 0.56), grid * 0.42);

        float radius = length(localGroundPosition);
        float rim = smoothstep(0.985 - fwidth(radius), 0.985 + fwidth(radius), radius);
        color = mix(color, vec3(0.64, 0.69, 0.60), rim);
    }
    FragColor = vec4(color, uColor.a);
}
