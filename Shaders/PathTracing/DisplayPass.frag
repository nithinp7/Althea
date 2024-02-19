#version 450

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

#define IS_RT_SHADER 0
#include <GlobalIllumination/GIResources.glsl>

void main() {
#if 1
  vec3 texSample = texture(colorTargetTx, uv).rgb;
#else  
  float prevDepth = texture(prevDepthTargetTx, uv).r;
  // prevDepth = fract(0.5 * log(prevDepth));
  prevDepth = mod(prevDepth, 10.0);
  float depth = texture(depthTargetTx, uv).r;
  // depth = fract(0.5 * log(depth));
  depth = mod(depth, 10.0);
  float diff = abs(depth - prevDepth);
  vec3 texSample = vec3(prevDepth + diff, prevDepth, prevDepth - diff);
  if (isnan(depth))
    texSample = vec3(1000., 0.0, 0.0);
#endif 

#ifndef SKIP_TONEMAP
  texSample = vec3(1.0) - exp(-texSample * globals.exposure);
  //texSample = texSample / (vec3(1.0) + texSample);
#endif

#if 0
  vec3 dbgSample = texture(dbg, (uv - vec2(0.75)) / 0.25).rgb;
  if (uv.x > 0.75 && uv.y > 0.75) {
    outColor = vec4(dbgSample, 1.0);
  } else 
#endif
  {
    outColor = vec4(texSample, 1.0);
  }
}