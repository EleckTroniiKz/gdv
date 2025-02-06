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

uniform vec3 viewPos;

out vec4 color;

void main() {
    // TODO: Ex 4.1a Implement phong lighting calculation
    // Normalize normal
    vec3 norm = normalize(vNormal);

    // Compute light direction
    vec3 lightDir = normalize(lightPosition - vPos);

    // Compute ambient component
    vec3 ambient = ambientColor * ambientIntensity;

    // Compute diffuse component
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseColor * diff * lightIntensity;

    // Compute specular component
    vec3 viewDir = normalize(viewPos - vPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularColor * spec * lightIntensity;

    // Combine lighting components
    vec3 result = ambient + diffuse + specular;
    color = vec4(result, 1.0);
}

