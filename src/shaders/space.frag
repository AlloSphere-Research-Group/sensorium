#version 330

uniform sampler2D texSpace;
uniform float blend;

in vec2 var_texcoord;

layout(location = 0) out vec4 fragColor;

void main() { fragColor = blend * texture(texSpace, var_texcoord); }