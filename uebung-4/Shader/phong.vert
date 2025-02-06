#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec3 vNormal;
out vec3 vPos;
out vec2 vTexCoord;

void main() {
    vPos = vec3(modelView * vec4(position, 1.0));
    vNormal = normalize(normalMatrix * normal);
    vTexCoord = texCoord;

    gl_Position = projection * modelView * vec4(position, 1.0);
}
