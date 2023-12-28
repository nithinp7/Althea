#version 460 core

layout(location=0) in vec3 worldPosCS;
layout(location=1) in vec2 baseColorUV;

layout(depth_any) out float gl_FragDepth;

#include "Global/GlobalResources.glsl"
#include "PrimitiveConstants.glsl"

SAMPLER2D(textureHeap);

layout(push_constant) uniform PushConstants {
  mat4 model;
  uint primIdx;
  uint lightIdx;
  uint globalResourcesHandle;
  uint pointLightBufferHandle;
  uint pointLightConstantsHandle;
} pushConstants;

#define resources RESOURCE(globalResources,pushConstants.globalResourcesHandle)

void main() {
  PrimitiveConstants primConstants = 
      RESOURCE(primitiveConstants, resources.primitiveConstantsBuffer)
        .primitiveConstantsArr[pushConstants.primIdx];
  
  // TODO: PARAMATERIZE NEAR / FAR
  float zNear = 0.01;
  float zFar = 1000.0;

  // Read opacity mask (alpha channel of base color)
  float alpha = 
      (primConstants.baseColorFactor * 
       TEXTURE_SAMPLE(textureHeap, primConstants.baseTextureHandle, baseColorUV)).a;
  if (alpha < primConstants.alphaCutoff) {
    discard;
  }

  gl_FragDepth = length(worldPosCS) / zFar;
}
