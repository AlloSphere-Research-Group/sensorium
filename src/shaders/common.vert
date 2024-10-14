#version 330
uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texcoord;

uniform float eye_sep;
uniform float foc_len;

out vec2 var_texcoord;

vec4 stereo_displace(vec4 v, float e, float r) {
  vec3 OE = vec3(-v.z, 0.0, v.x); // eye dir, orthogonal to vert vec
  OE = normalize(OE);             // but preserving +y up-vector
  OE *= e;                        // set mag to eye separation
  vec3 EV = v.xyz - OE;           // eye to vertex
  float ev = length(EV);          // save length
  EV /= ev;                       // normalize

  // coefs for polynomial t^2 + 2bt + c = 0
  // derived from cosine law r^2 = t^2 + e^2 + 2tecos(theta)
  // where theta is angle between OE and EV
  // t is distance to sphere surface from eye
  float b = -dot(OE, EV); // multiply -1 to dot product cuz
                          // OE needs to be flipped in dir
  float c = e * e - r * r;
  float t = -b + sqrt(b * b - c); // quadratic formula

  v.xyz = OE + t * EV;           // dir from orig to sphere surface
  v.xyz = ev * normalize(v.xyz); // set mag to eye-to-v dist
  return v;
}

void main() {
  gl_Position = al_ProjectionMatrix *
                stereo_displace(al_ModelViewMatrix * vec4(position, 1.0),
                                eye_sep, foc_len);

  var_texcoord = texcoord;
}