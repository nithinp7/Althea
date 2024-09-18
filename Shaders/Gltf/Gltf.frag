
#version 460 core

#define PI 3.14159265359

layout(location=0) in vec2 baseColorUV;
layout(location=1) in vec2 normalMapUV;
layout(location=2) in vec2 metallicRoughnessUV;
layout(location=3) in vec2 occlusionUV;
layout(location=4) in vec2 emissiveUV;
layout(location=5) in mat3 fragTBN;
layout(location=8) in vec3 worldPos;
layout(location=9) in vec3 direction;

layout(location=0) out vec4 GBuffer_Normal;
layout(location=1) out vec4 GBuffer_Albedo;
layout(location=2) out vec4 GBuffer_MetallicRoughnessOcclusion;

#include <Bindless/GlobalHeap.glsl>
#include <Global/GlobalUniforms.glsl>
#include <Global/GlobalResources.glsl>
#include <PrimitiveResources.glsl>

layout(push_constant) uniform PushConstants {
  uint matrixBufferHandle;
  uint primConstantsBuffer;
  uint globalResourcesHandle;
  uint globalUniformsHandle;
} pushConstants;

SAMPLER2D(textureHeap);

#define globals RESOURCE(globalUniforms, pushConstants.globalUniformsHandle)
#define resources RESOURCE(globalResources, pushConstants.globalResourcesHandle)

void main() {
  PrimitiveConstants constants = 
      RESOURCE(primitiveConstants, pushConstants.primConstantsBuffer)
        .primitiveConstantsArr[0];

  GBuffer_Albedo = 
      TEXTURE_SAMPLE(textureHeap, constants.baseTextureHandle, baseColorUV) 
      * constants.baseColorFactor;

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

  GBuffer_MetallicRoughnessOcclusion = vec4(metallicRoughness, 0.0, GBuffer_Albedo.a);

  // vec4 emissive = TEXTURE_SAMPLE(textureHeap, constants.emissiveTextureHandle, emissiveUV);
  // GBuffer_Albedo.rgb = emissive.rgb;
  
  if (GBuffer_Albedo.a < constants.alphaCutoff) {
    discard;
  }
}
