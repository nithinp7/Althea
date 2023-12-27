
#version 450

#define PI 3.14159265359

layout(location=0) in vec2 baseColorUV;
layout(location=1) in vec2 normalMapUV;
layout(location=2) in vec2 metallicRoughnessUV;
layout(location=3) in vec2 occlusionUV;
layout(location=4) in vec2 emissiveUV;
layout(location=5) in mat3 fragTBN;
layout(location=8) in vec3 worldPos;
layout(location=9) in vec3 direction;

layout(location=0) out vec4 GBuffer_Position;
layout(location=1) out vec4 GBuffer_Normal;
layout(location=2) out vec4 GBuffer_Albedo;
layout(location=3) out vec4 GBuffer_MetallicRoughnessOcclusion;

#include "Bindless/GlobalHeap.glsl"
#include "Global/GlobalUniforms.glsl"
#include "Global/GlobalResources.glsl"
#include "PrimitiveConstants.glsl"

layout(push_constant) uniform PushConstants {
  mat4 model;
  uint primId;
  uint globalResourcesHandle;
  uint globalUniformsHandle;
} pushConstants;

SAMPLER2D(textureHeap);

void main() {
  GlobalUniforms globals = RESOURCE(globalUniforms, pushConstants.globalUniformsHandle);
  GlobalResources resources = RESOURCE(globalResources, pushConstants.globalResourcesHandle);

  PrimitiveConstants constants = 
      RESOURCE(primitiveConstants, resources.primitiveConstantsBuffer)
        .primitiveConstantsArr[primId];

  GBuffer_Albedo = 
      TEXTURE_SAMPLE(textureHeap, constants.baseTextureHandle, baseColorUV) 
      * constants.baseColorFactor;

  GBuffer_Position = vec4(worldPos, GBuffer_Albedo.a);

  vec3 normalMapSample = 
      TEXTURE_SAMPLE(textureHeap, constants.normalTextureHandle, normalMapUV).rgb;
  vec3 tangentSpaceNormal = 
      (2.0 * normalMapSample - 1.0) *
      vec3(constants.normalScale, constants.normalScale, 1.0);
  GBuffer_Normal = vec4(normalize(fragTBN * tangentSpaceNormal), GBuffer_Albedo.a);

  vec2 metallicRoughness = 
      TEXTURE_SAMPLE(textureHeap, constants.metallicRoughnessTextureHandle, metallicRoughnessUV).bg *
      vec2(constants.metallicFactor, constants.roughnessFactor);
  float ambientOcclusion = 
      TEXTURE_SAMPLE(textureHeap, constants.occlusionTextureHandle, occlusionUV).r * constants.occlusionStrength;

  GBuffer_MetallicRoughnessOcclusion = vec4(metallicRoughness, ambientOcclusion, GBuffer_Albedo.a);
  
  if (GBuffer_Albedo.a < constants.alphaCutoff) {
    discard;
  }
}
