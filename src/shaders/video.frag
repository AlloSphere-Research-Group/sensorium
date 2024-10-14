#version 330
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
uniform float videoBlend;

in vec2 var_texcoord;
layout(location = 0) out vec4 fragColor;

void main() {
  vec3 yuv;
  yuv.r = texture(texY, var_texcoord).r - 0.0625;
  yuv.g = texture(texU, var_texcoord).r - 0.5;
  yuv.b = texture(texV, var_texcoord).r - 0.5;

  vec4 rgba;
  rgba.r = yuv.r + 1.596 * yuv.b;
  rgba.g = yuv.r - 0.813 * yuv.b - 0.391 * yuv.g;
  rgba.b = yuv.r + 2.018 * yuv.g;
  rgba.a = 1.0;

  // adjust for co2?
  // video.a = (video.r + video.g + video.b) / 1.35;
  fragColor = videoBlend * rgba;
}