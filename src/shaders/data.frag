#version 330

uniform sampler2D texEarth;
uniform sampler2D texData;
uniform float validData;
uniform float blend;

in vec2 var_texcoord;

layout(location = 0) out vec4 fragColor;

void main() {
  vec4 earthPixel = texture(texEarth, var_texcoord);

  vec4 pixel = vec4(0);
  pixel += validData * texture(texData, var_texcoord);
  pixel.a = step(0.1f, pixel.r + pixel.g + pixel.b);

  pixel = mix(earthPixel, pixel, pixel.a);

  fragColor = blend * pixel;
}