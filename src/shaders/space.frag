#version 330

uniform sampler2D texSpace;
uniform float dataBlend;

in vec2 var_texcoord;

layout(location = 0) out vec4 fragColor;

void main() { fragColor = dataBlend * texture(texSpace, var_texcoord); }