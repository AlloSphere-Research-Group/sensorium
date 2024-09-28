#version 330

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 vertexTexcoord;
out vec2 vTexcoord;

void main() {

  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * vec4(position, 1.0);

  vTexcoord = vertexTexcoord;
}