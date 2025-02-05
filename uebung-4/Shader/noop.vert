#version 330 core

layout(location = 0) in vec3 position; //Vertex position in model coordinates
layout(location = 3) in vec2 texCoord; //Texture coordinate (for using textures)

out vec2 vTexCoord; //Texture coordinate of current vertex

void main() {
    gl_Position = vec4(position, 1.0);
    vTexCoord = texCoord;
}
