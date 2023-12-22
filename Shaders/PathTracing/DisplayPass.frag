#version 450

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <GlobalUniforms.glsl>

layout(set=1, binding=0) uniform sampler2D tex; 
layout(set=1, binding=1) uniform sampler2D dbg;

void main() {
  vec3 texSample = texture(tex, uv).rgb;

#ifndef SKIP_TONEMAP
  texSample = vec3(1.0) - exp(-texSample * globals.exposure);
  //texSample = texSample / (vec3(1.0) + texSample);
#endif

#if 1
  vec3 dbgSample = texture(dbg, (uv - vec2(0.75)) / 0.25).rgb;
  if (uv.x > 0.75 && uv.y > 0.75) {
    outColor = vec4(dbgSample, 1.0);
  } else 
#endif
  {
    outColor = vec4(texSample, 1.0);
  }
}