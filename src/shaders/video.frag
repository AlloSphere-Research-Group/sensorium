#version 330

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;

in vec2 vTexcoord;

layout(location = 0) out vec4 fragColor;

void main() {
  vec3 yuv;
  yuv.r = texture(tex0, vTexcoord).r - 0.0625;
  yuv.g = texture(tex1, vTexcoord).r - 0.5;
  yuv.b = texture(tex2, vTexcoord).r - 0.5;

  vec4 video;
  video.r = yuv.r + 1.596 * yuv.b;
  video.g = yuv.r - 0.813 * yuv.b - 0.391 * yuv.g;
  video.b = yuv.r + 2.018 * yuv.g;
  // video.a = 1.0;
  // video.a = (video.r + video.r + video.g + video.g + video.g + video.b) / 6;
  video.a = (video.r + video.g + video.b) / 1.35;

  fragColor = video;
}