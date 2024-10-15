#version 330

uniform sampler2D texEarth;
uniform sampler2D texCloud;
uniform sampler3D texData;
uniform float validData;
uniform float index;
uniform float dataBlend;
uniform float showCloud;

in vec2 var_texcoord;

layout(location = 0) out vec4 fragColor;

void main() {
  vec4 earthPixel = texture(texEarth, var_texcoord);
  vec4 cloudPixel = showCloud * texture(texCloud, var_texcoord);
  earthPixel = clamp(earthPixel + cloudPixel, 0.f, 1.f);

  vec3 tex3Dcoord = vec3(var_texcoord, index);
  vec4 pixel = vec4(0);
  pixel += validData * texture(texData, tex3Dcoord);
  pixel.a = step(0.1f, pixel.r + pixel.g + pixel.b);

  pixel = mix(earthPixel, pixel, pixel.a);

  fragColor = dataBlend * pixel;
}