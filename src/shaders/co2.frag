#version 330
uniform sampler2D texEarth;
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
uniform float dataBlend;

in vec2 var_texcoord;
layout(location = 0) out vec4 fragColor;

void main() {
  vec4 earthPixel = texture(texEarth, var_texcoord);

  vec3 yuv;
  yuv.r = texture(texY, var_texcoord).r - 0.0625;
  yuv.g = texture(texU, var_texcoord).r - 0.5;
  yuv.b = texture(texV, var_texcoord).r - 0.5;

  vec4 pixel;
  pixel.r = yuv.r + 1.596 * yuv.b;
  pixel.g = yuv.r - 0.813 * yuv.b - 0.391 * yuv.g;
  pixel.b = yuv.r + 2.018 * yuv.g;
  pixel.a = (pixel.r + pixel.g + pixel.b) / 1.35;

  pixel = mix(earthPixel, pixel, pixel.a);

  fragColor = dataBlend * pixel;
}