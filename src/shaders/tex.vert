#version 330

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

layout(location = 0) in vec3 position;
layout (location = 2) in vec2 vertexTexcoord;
out vec2 vTexcoord;

uniform float eye_sep;
uniform float foc_len;

// uniform float fade;
// out vec4 color_;

vec4 stereo_displace(vec4 v, float e, float r) {
  vec3 OE =
      vec3(-v.z, 0.0,
           v.x);      // eye direction, orthogonal to vertex vector // right vec
  OE = normalize(OE); // but preserving +y up-vector
  OE *= e;            // set mag to eye separation
  vec3 EV = v.xyz - OE;  // eye to vertex
  float ev = length(EV); // save length
  EV /= ev;              // normalize

  // coefs for polynomial t^2 + 2bt + c = 0
  // derived from cosine law r^2 = t^2 + e^2 + 2tecos(theta)
  // where theta is angle between OE and EV
  // t is distance to sphere surface from eye
  float b = -dot(OE, EV); // multiply -1 to dot product because
                          // OE needs to be flipped in direction
  float c = e * e - r * r;
  float t = -b + sqrt(b * b - c); // quadratic formula

  v.xyz = OE + t * EV;           // direction from origin to sphere surface
  v.xyz = ev * normalize(v.xyz); // normalize and set mag to eye-to-v distance
  return v;
}

// not omni
// vec4 stereo_displace(vec4 v, float e, float f) {
//   // eye to vertex distance
//   float l = sqrt((v.x - e) * (v.x - e) + v.y * v.y + v.z * v.z);
//   // absolute z-direction distance
//   float z = abs(v.z);
//   // x coord of projection of vertex on focal plane when looked from eye
//   float t = f * (v.x - e) / z;
//   // x coord of displaced vertex to make displaced vertex be projected on
//   focal plane
//   // when looked from origin at the same point original vertex would be
//   projected
//   // when looked form eye
//   v.x = z * (e + t) / f;
//   // set distance from origin to displaced vertex same as eye to original
//   vertex v.xyz = normalize(v.xyz); v.xyz *= l; return v;
// }

void main() {

  float e = eye_sep;
  float f = foc_len;
  // e = -e;
  // f = 2.5;

  if (eye_sep == 0) {
    gl_Position =
        al_ProjectionMatrix * al_ModelViewMatrix * vec4(position, 1.0);
  } else {
    gl_Position =
        al_ProjectionMatrix *
        stereo_displace(al_ModelViewMatrix * vec4(position, 1.0), e, f);
  }

  vTexcoord = vertexTexcoord;
}