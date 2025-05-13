#version 330 core

in vec3 vNormal;   //Per-vertex normal, transformed
in vec3 vPos;      //Position in camera coordinates
in vec2 vTexCoord; //Texture coordinate of current vertex

//Material parameters
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float shininess;

//Light parameters
uniform vec3 lightPosition;         //Position of the light in camera coordinates
uniform float ambientIntensity;
uniform float lightIntensity;

out vec4 color;

void main() {
    // TODO: Ex 4.1a Implement phong lighting calculation
    color = vec4(1.0);
}

