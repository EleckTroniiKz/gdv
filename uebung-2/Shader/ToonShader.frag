varying float intensity;

void main()
{
	vec4 color;
	if (intensity > 0.95)
		color = vec4(1.0,0.0,0.0,1.0);
	else if (intensity > 0.5)
		color = vec4(0.0,1.0,0.0,1.0);
	else if (intensity > 0.25)
		color = vec4(0.0,0.0,1.0,1.0);
	else
		color = vec4(1.0,0.1,1.0,1.0);
	gl_FragColor = color;

}