#version 330 core

/*
This vertex shader calculates a simple modelView and projection transformation according to the provided matrices. It will further passthrough several per-vertex attributes to the next shader stage. Please note that OpenGL automatically interpolates vertex outputs for the fragment shader.
*/

layout(location = 0) in vec3 position; //Vertex position in model coordinates
layout(location = 1) in vec3 normal;   //Vertex normal
layout(location = 3) in vec2 texCoord; //Texture coordinate (for using textures)

uniform mat4 model;         //Model matrix
uniform mat4 modelView;     //ModelView matrix
uniform mat4 projection;    //Projection matrix
uniform mat3 normalMatrix;  //The transpose inverse of the ModelView matrix, used for transformation of normals.
uniform mat4 lightMatrix;   //ModelViewProjection Matrix of light

out vec3 vNormal;   //Per-vertex normal, transformed
out vec3 vPos;      //Position in camera coordinates
out vec4 lightProjectedPosition;
out vec2 vTexCoord; //Texture coordinate of current vertex

void main() {
    lightProjectedPosition = lightMatrix * vec4(position, 1.0);
    gl_Position = projection * modelView * vec4(position, 1.0);
    vec4 tempPos = modelView * vec4(position, 1.0);
    vPos = tempPos.xyz / tempPos.w; //inhomogenous coordinates
    vNormal = normalMatrix * normal;
    vTexCoord = texCoord;
}
