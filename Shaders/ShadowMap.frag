
#version 450

layout(location=0) in vec3 worldPosCS;
layout(location=1) in vec2 baseColorUV;

layout(depth_any) out float gl_FragDepth;

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
} constants;

layout(set=1, binding=1) uniform sampler2D baseColorTexture;


// TODO: 
void main() {
  // TODO: PARAMATERIZE NEAR / FAR
  float zNear = 0.01;
  float zFar = 1000.0;

  // Read opacity mask (alpha channel of base color)
  float alpha = 
      (texture(baseColorTexture, baseColorUV) * constants.baseColorFactor).a;
  if (alpha < constants.alphaCutoff) {
    discard;
  }

  gl_FragDepth = length(worldPosCS) / zFar;
}
