#ifndef _PRIMITIVECONSTANTS_
#define _PRIMITIVECONSTANTS_

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

  uint padding;
};

BUFFER_R(primitiveConstants, PrimitiveConstantsResource{
  PrimitiveConstants primitiveConstantsArr[];
});

#endif // _PRIMITIVECONSTANTS_