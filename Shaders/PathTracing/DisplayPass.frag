#version 450

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

#define IS_RT_SHADER 0
#include <GlobalIllumination/GIResources.glsl>

void main() {
#if 0
  vec3 texSample = texture(colorTargetTx, uv).rgb;
#else  
  uvec2 pixelPos = uvec2(uv.x * giUniforms.targetWidth, uv.y * giUniforms.targetHeight);
  uint reservoirIdx = pixelPos.x * giUniforms.targetHeight + pixelPos.y;
  vec3 texSample = vec3(0.0);
  float wSum = getReservoir(reservoirIdx).wSum;
  for (int i = 0; i < 8; ++i) {
    texSample += 
        0.125 * getReservoir(reservoirIdx).samples[i].radiance 
        * getReservoir(reservoirIdx).samples[i].W / wSum;
  }
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