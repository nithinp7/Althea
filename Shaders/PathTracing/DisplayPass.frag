#version 450

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

#define IS_RT_SHADER 0
#include <GlobalIllumination/GIResources.glsl>

void main() {
  vec3 texSample = texture(colorTargetTx, uv).rgb;

  outColor = vec4(texSample, 1.0);
}