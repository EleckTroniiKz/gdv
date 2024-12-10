varying vec3 normal;
varying vec3 lightDir;
varying vec3 viewDir;

void main()
{
    vec3 transformedNormal = normalize(gl_NormalMatrix * gl_Normal);
    vec4 transformedPosition = gl_ModelViewMatrix * gl_Vertex;

    normal = transformedNormal;
    lightDir = normalize(vec3(gl_LightSource[0].position) - transformedPosition.xyz);
    viewDir = normalize(-transformedPosition.xyz);

    gl_Position = ftransform();
}
