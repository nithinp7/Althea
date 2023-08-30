
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

#ifndef TEXTURE_HEAP_COUNT
#define TEXTURE_HEAP_COUNT 1
#endif

layout(set=0, binding=7) uniform sampler2D textureHeap[TEXTURE_HEAP_COUNT];

layout(set=1, binding=0) uniform ConstantBufferObject {
  vec4 baseColorFactor;
  vec3 emissiveFactor;

  int baseTextureCoordinateIndex;
  int normalMapTextureCoordinateIndex;
  int metallicRoughnessTextureCoordinateIndex;
  int occlusionTextureCoordinateIndex;
  int emissiveTextureCoordinateIndex;

  float normalScale;
  float metallicFactor;
  float roughnessFactor;
  float occlusionStrength;

  float alphaCutoff;

  int primId;
} constants;

// layout(set=1, binding=1) uniform sampler2D baseColorTexture;
// layout(set=1, binding=2) uniform sampler2D normalMapTexture;
// layout(set=1, binding=3) uniform sampler2D metallicRoughnessTexture;
// layout(set=1, binding=4) uniform sampler2D occlusionTexture;
// layout(set=1, binding=5) uniform sampler2D emissiveTexture;

#define baseColorTexture textureHeap[5*constants.primId+0]
#define normalMapTexture textureHeap[5*constants.primId+1]
#define metallicRoughnessTexture textureHeap[5*constants.primId+2]
#define occlusionTexture textureHeap[5*constants.primId+3]
#define emissiveTexture textureHeap[5*constants.primId+4]

void main() {
  GBuffer_Albedo = texture(baseColorTexture, baseColorUV) * constants.baseColorFactor;

  GBuffer_Position = vec4(worldPos, GBuffer_Albedo.a);

  vec3 normalMapSample = texture(normalMapTexture, normalMapUV).rgb;
  vec3 tangentSpaceNormal = 
      (2.0 * normalMapSample - 1.0) *
      vec3(constants.normalScale, constants.normalScale, 1.0);
  GBuffer_Normal = vec4(normalize(fragTBN * tangentSpaceNormal), GBuffer_Albedo.a);

  vec2 metallicRoughness = 
      texture(metallicRoughnessTexture, metallicRoughnessUV).bg *
      vec2(constants.metallicFactor, constants.roughnessFactor);
  float ambientOcclusion = 
      texture(occlusionTexture, occlusionUV).r * constants.occlusionStrength;

  GBuffer_MetallicRoughnessOcclusion = vec4(metallicRoughness, ambientOcclusion, GBuffer_Albedo.a);
  
  if (GBuffer_Albedo.a < constants.alphaCutoff) {
    discard;
  }
}
