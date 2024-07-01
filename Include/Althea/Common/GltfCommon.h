#ifndef _GLTFCOMMON_
#define _GLTFCOMMON_

#include <../Include/Althea/Common/CommonTranslations.h>

struct PrimitiveConstants {
  vec4 baseColorFactor;
  vec4 emissiveFactor;

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
  uint baseTextureHandle;
  uint normalTextureHandle;

  uint metallicRoughnessTextureHandle;
  uint occlusionTextureHandle;
  uint emissiveTextureHandle;
  uint vertexBufferHandle;

  uint indexBufferHandle;
  uint jointMapHandle;
  uint nodeIdx;
  uint isSkinned; // TODO: turn this into a flag bitmask
};

struct Vertex {
  vec3 position;
  vec3 tangent;
  vec3 bitangent;
  vec3 normal;
  vec2 uvs[4];
  vec4 weights;
  u16vec4 joints;
};

#endif // _GLTFCOMMON_