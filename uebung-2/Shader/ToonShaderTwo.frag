varying float intensity;

void main()
{
    vec4 color;
    float pseudoTime = mod(gl_FragCoord.x + gl_FragCoord.y, 100.0) * 0.01; // Creates a pseudo-time
    float dynamicFactor = abs(sin(pseudoTime)); // Oscillates the color factor

    if (intensity > 0.95)
        color = vec4(1.0 * dynamicFactor, 0.5, 0.5, 1.0);
    else if (intensity > 0.5)
        color = vec4(0.6 * dynamicFactor, 0.3, 0.3, 1.0);
    else if (intensity > 0.25)
        color = vec4(0.4 * dynamicFactor, 0.2, 0.2, 1.0);
    else
        color = vec4(0.2 * dynamicFactor, 0.1, 0.1, 1.0);

    gl_FragColor = color;
}
