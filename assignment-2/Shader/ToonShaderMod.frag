varying float intensity;

void main()
{
    vec4 color;
    float dynFac = mod(gl_FragCoord.x + gl_FragCoord.y, 100.0) * 0.01;

    if (intensity > 0.95)
        color = vec4(1.0 * dynFac, 0.5, 0.5, 1.0);
    else if (intensity > 0.5)
        color = vec4(0.6 * dynFac, 0.3, 0.3, 1.0);
    else if (intensity > 0.25)
        color = vec4(0.4 * dynFac, 0.2, 0.2, 1.0);
    else
        color = vec4(0.2 * dynFac, 0.1, 0.1, 1.0);

    gl_FragColor = color;
}
