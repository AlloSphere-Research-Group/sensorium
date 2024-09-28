#version 330

uniform sampler2D tex0;

in vec2 vTexcoord;

layout(location = 0) out vec4 fragColor;

void main() { fragColor = texture(tex0, vTexcoord); }