#version 450

#include "SimpleRTResources.glsl"

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

void main() {
  vec3 texSample = texture(rtTargetTex, uv).rgb;
  // vec3 texSample = texture(gBufferNormal, uv).rgb;
  outColor = vec4(texSample, 1.0);
}