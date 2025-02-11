#version 330 core

layout (location = 0) in vec3 aPos;
// matrices to project vertex coords from point of view of light
uniform mat4 lightSpaceMatrix; 
uniform mat4 model;

void main()
{
    // calculate projectioncoordinate in lightspace
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
