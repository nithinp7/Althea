
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

  float padding1;
  float padding2;
};

layout(std430, set=PRIMITIVE_CONSTANTS_SET, binding=PRIMITIVE_CONSTANTS_BINDING) readonly buffer PRIMITIVE_CONSTANTS {
  PrimitiveConstants primitiveConstants[];
};