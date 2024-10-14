#version 330

uniform sampler2D tex0;

in vec2 var_texcoord;

layout(location = 0) out vec4 fragColor;

void main() { fragColor = texture(tex0, var_texcoord); }