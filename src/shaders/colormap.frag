#version 330

uniform sampler2D tex0;
uniform int mapFunction = 0;
uniform int isVideo = 0;
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;

in vec2 vTexcoord;

layout (location = 0) out vec4 fragColor;

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    
  vec4 pixel;
  if(isVideo == 1){
    vec3 yuv0;
    yuv0.r = texture(texY, vTexcoord).r - 0.0625;
    yuv0.g = texture(texU, vTexcoord).r - 0.5;
    yuv0.b = texture(texV, vTexcoord).r - 0.5;

    vec4 c;
    c.r = yuv0.r + 1.596 * yuv0.b;
    c.g = yuv0.r - 0.813 * yuv0.b - 0.391 * yuv0.g;
    c.b = yuv0.r + 2.018 * yuv0.g;
    // c.a = 1.0;
    // c.a = (c.r+c.r+c.g+c.g+c.g+c.b) / 6;
    c.a = (c.r + c.g + c.b) / 1.35;

    pixel = c;
  } else {
    vec4 c = texture(tex0, vTexcoord);
    // c.a = (c.r+c.r+c.g+c.g+c.g+c.b) / 6;
    c.a = (c.r + c.g + c.b) / 3;
    pixel = c;
  }

  // if(pixel.r == 0.0)
  //   discard;

  // vec3 hsv;
  // if(mapFunction == 0){ // sst
  //   // data_color = HSV(0.55 + log(pixel.r / 90. + 1), 0.65 + pixel.r / 60, 0.6 + atan(pixel.r / 300));
  //   hsv = vec3(0.55 + log(pixel.r / (90./255.) + 1), 0.65 + pixel.r / (60./255.), 0.6 + atan(pixel.r / (300./255.)));
  
  // }else if(mapFunction == 1){ // nutrient
  //   // data_color = HSV(0.3 - log(pixel.r / 60. + 1), 0.9 + pixel.r / 90, 0.9 + pixel.r / 90);
  //   hsv = vec3(0.3 + log(pixel.r / (60./255.) + 1), 0.9 + pixel.r / (90./255.), 0.9 + pixel.r / (90./255.) );
  
  // }else if(mapFunction == 2){ // shipping
  //   // data_color = HSV(1 - log(pixel.r / 30. + 1), 0.6 + pixel.r / 100, 0.6 + pixel.r / 60);
  //   hsv = vec3(1. - log(pixel.r / (30./255.) + 1), 0.6 + pixel.r / (100./255.), 0.6 + pixel.r / (60./255.));
  
  // }else if(mapFunction == 3){ // acidification
  //   // data_color = HSV(0.7 - 0.6 * log(pixel.r / 100. + 1), 0.5 + log(pixel.r / 100. + 1), 1);
  //   hsv = vec3(0.7 - 0.6 * log(pixel.r / (100./255.) + 1), 0.5 + log(pixel.r / (100./255.) + 1.), 1.);
  
  // }else if(mapFunction == 4){ // sea level rise
  //   // data_color = HSV(0.6 + 0.2 * log(pixel.r / 100. + 1), 0.6 + log(pixel.r / 60. + 1), 0.6 + log(pixel.r / 60. + 1));
  //   hsv = vec3(0.6 + 0.2 * log(pixel.r / (100./255.) + 1), 0.6 + log(pixel.r / (60./255.) + 1.), 0.6 + log(pixel.r / (60./255.) + 1.));

  // }else if(mapFunction >= 5 && mapFunction <= 8){ // fishing
  //   // data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);
  //   hsv = vec3(log(pixel.r / (90./255.) + 1), 0.9, 1);
  // }else if(mapFunction >= 9 && mapFunction <= 11){ // 9,10,11
  //   // data_color = HSV(log(pixel.r / 120. + 1), 0.9, 1);
  //   hsv = vec3(log(pixel.r / (120./255.) + 1), 0.9, 1);
  // }


  // vec3 c = hsv2rgb(hsv);
  // fragColor = vec4(c, pixel.r*10.0);

  fragColor = vec4(pixel);
}