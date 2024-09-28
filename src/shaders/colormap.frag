#version 330

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform float dataNum;

in vec2 vTexcoord;

layout(location = 0) out vec4 fragColor;

void main() {
  vec4 pixel;

  vec4 image0 = texture(tex0, vTexcoord);
  image0.a = (image0.r + image0.g + image0.b) / 3;
  image0.rgb *= image0.a * (1.0 - step(dataNum, 0));
  vec4 image1 = texture(tex1, vTexcoord);
  image1.a = (image1.r + image1.g + image1.b) / 3;
  image1.rgb *= image1.a * (1.0 - step(dataNum, 1));
  vec4 image2 = texture(tex2, vTexcoord);
  image2.a = (image2.r + image2.g + image2.b) / 3;
  image2.rgb *= image2.a * (1.0 - step(dataNum, 2));
  vec4 image3 = texture(tex3, vTexcoord);
  image3.a = (image3.r + image3.g + image3.b) / 3;
  image3.rgb *= image3.a * (1.0 - step(dataNum, 3));

  fragColor = image0 + image1 + image2 + image3;
}