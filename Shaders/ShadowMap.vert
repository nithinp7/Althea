#version 450

#ifdef POINT_LIGHTS_BINDLESS
#include "ShadowMapBindless.vert"
#else 

#extension GL_EXT_multiview : enable

layout(location=0) in vec3 position;
layout(location=1) in mat3 tbn;
// layout(location = 1) in vec3 tangent;
// layout(location = 2) in vec3 bitangent;
// layout(location = 3) in vec3 normal;
layout(location=4) in vec2 uvs[4];

layout(location=0) out vec3 worldPosCS;
layout(location=1) out vec2 baseColorUV;

layout(push_constant) uniform PushConstants {
  mat4 model;
} pushConstants;

layout(set=0, binding=0) uniform UniformBufferObject {
  mat4 projection;
  mat4 inverseProjection;
  mat4 views[6];
  mat4 inverseViews[6];
} globals;

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

void main() {
  vec4 csPos = globals.views[gl_ViewIndex] * pushConstants.model * vec4(position, 1.0);
  gl_Position = globals.projection * csPos;
  
  worldPosCS = csPos.xyz / csPos.w;
  baseColorUV = uvs[constants.baseTextureCoordinateIndex];
}

#endif