#ifndef _GLOBALUNIFORMS_
#define _GLOBALUNIFORMS_

#include <Bindless/GlobalHeap.glsl>

#extension GL_EXT_scalar_block_layout : enable

UNIFORM_BUFFER(globalUniforms, GlobalUniforms{
  mat4 projection;
  mat4 inverseProjection;
  mat4 view;
  mat4 prevView;
  mat4 inverseView;
  mat4 prevInverseView;

  int lightCount;
  uint lightBufferHandle;
  float time;
  float exposure;

  vec2 mouseUV;
  uint inputMask;
  uint padding;
});

#endif // _GLOBALUNIFORMS_