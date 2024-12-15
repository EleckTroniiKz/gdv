varying vec3 normal;
varying vec3 lightDir;
varying vec3 viewDir;

void main()
{
    vec3 n = normalize(normal);
    vec3 l = normalize(lightDir);
    vec3 v = normalize(viewDir);

    float intensity = dot(n, l);

    // Toon
    vec4 color;
    if (intensity > 0.95)
        color = vec4(1.0, 0.8, 0.6, 1.0); // Bright tone
    else if (intensity > 0.5)
        color = vec4(0.8, 0.6, 0.4, 1.0); // Mid tone
    else if (intensity > 0.25)
        color = vec4(0.6, 0.4, 0.2, 1.0); // Dark tone
    else
        color = vec4(0.3, 0.2, 0.1, 1.0); // Shadow

    // Outline
    float edge = dot(n, v); // View direction vs. normal
    if (edge < 0.2) // Adjust threshold for stronger outlines
        color = vec4(0.1, 0.1, 0.1, 1.0); // Dark gray outline

    vec4 ambientBackground = vec4(0.1, 0.1, 0.2, 1.0); // Dark blue-ish background
    

    gl_FragColor = color;
}
