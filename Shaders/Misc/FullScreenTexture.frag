#version 450

#define PI 3.14159265359

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

#include <Global/GlobalUniforms.glsl>

layout(set=1, binding=0) uniform sampler2D tex; 

void main() {
  vec3 texSample = texture(tex, uv).rgb;

#ifndef SKIP_TONEMAP
  texSample = vec3(1.0) - exp(-texSample * globals.exposure);
  //texSample = texSample / (vec3(1.0) + texSample);
#endif

  outColor = vec4(texSample, 1.0);
}