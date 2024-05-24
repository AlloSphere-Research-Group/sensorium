#version 330

uniform sampler2D tex0;
in vec2 vTexcoord;

layout (location = 0) out vec4 fragColor;

uniform int mapFunction = 0;

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    
  vec4 pixel = texture(tex0, vTexcoord);

  if(pixel.r == 0.0)
    discard;

  vec3 hsv;
  if(mapFunction == 0){ // sst
    // data_color = HSV(0.55 + log(pixel.r / 90. + 1), 0.65 + pixel.r / 60, 0.6 + atan(pixel.r / 300));
    hsv = vec3(0.55 + log(pixel.r / (90./255.) + 1), 0.65 + pixel.r / (60./255.), 0.6 + atan(pixel.r / (300./255.)));
  
  }else if(mapFunction == 1){ // nutrient
    // data_color = HSV(0.3 - log(pixel.r / 60. + 1), 0.9 + pixel.r / 90, 0.9 + pixel.r / 90);
    hsv = vec3(0.3 + log(pixel.r / (60./255.) + 1), 0.9 + pixel.r / (90./255.), 0.9 + pixel.r / (90./255.) );
  
  }else if(mapFunction == 2){ // shipping
    // data_color = HSV(1 - log(pixel.r / 30. + 1), 0.6 + pixel.r / 100, 0.6 + pixel.r / 60);
    hsv = vec3(1. - log(pixel.r / (30./255.) + 1), 0.6 + pixel.r / (100./255.), 0.6 + pixel.r / (60./255.));
  
  }else if(mapFunction == 3){ // acidification
    // data_color = HSV(0.7 - 0.6 * log(pixel.r / 100. + 1), 0.5 + log(pixel.r / 100. + 1), 1);
    hsv = vec3(0.7 - 0.6 * log(pixel.r / (100./255.) + 1), 0.5 + log(pixel.r / (100./255.) + 1.), 1.);
  
  }else if(mapFunction == 4){ // sea level rise
    // data_color = HSV(0.6 + 0.2 * log(pixel.r / 100. + 1), 0.6 + log(pixel.r / 60. + 1), 0.6 + log(pixel.r / 60. + 1));
    hsv = vec3(0.6 + 0.2 * log(pixel.r / (100./255.) + 1), 0.6 + log(pixel.r / (60./255.) + 1.), 0.6 + log(pixel.r / (60./255.) + 1.));

  }else if(mapFunction >= 5 && mapFunction <= 8){ // fishing
    // data_color = HSV(log(pixel.r / 90. + 1), 0.9, 1);
    hsv = vec3(log(pixel.r / (90./255.) + 1), 0.9, 1);
  }else if(mapFunction >= 9 && mapFunction <= 11){ // 9,10,11
    // data_color = HSV(log(pixel.r / 120. + 1), 0.9, 1);
    hsv = vec3(log(pixel.r / (120./255.) + 1), 0.9, 1);
  }


  vec3 c = hsv2rgb(hsv);
  fragColor = vec4(c, pixel.r*10.0);
  // fragColor = vec4(c, 1.0);
}