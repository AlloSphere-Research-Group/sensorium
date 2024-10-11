#version 330

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform float dataNum;

in vec2 var_texcoord;

layout(location = 0) out vec4 fragColor;

void main() {
  vec4 pixel;

  vec4 image0 = texture(tex0, var_texcoord);
  image0 *= 1.0 - step(dataNum, 0);
  vec4 image1 = texture(tex1, var_texcoord);
  image1 *= 1.0 - step(dataNum, 1);
  vec4 image2 = texture(tex2, var_texcoord);
  image2 *= 1.0 - step(dataNum, 2);
  vec4 image3 = texture(tex3, var_texcoord);
  image3 *= 1.0 - step(dataNum, 3);

  fragColor = (image0 + image1 + image2 + image3); // / dataNum;
}