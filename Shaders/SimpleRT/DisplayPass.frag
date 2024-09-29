#version 450

#include "SimpleRTResources.glsl"

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

void main() {
  vec3 texSample = texture(rtTargetTex, uv).rgb;
  
  texSample = vec3(1.0) - exp(-texSample * globals.exposure);

  outColor = vec4(texSample, 1.0);
}