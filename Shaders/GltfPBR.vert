
#version 450

layout(location=0) in vec3 position;
layout(location=1) in mat3 tbn;
// layout(location = 1) in vec3 tangent;
// layout(location = 2) in vec3 bitangent;
// layout(location = 3) in vec3 normal;
layout(location=4) in vec2 uvs[4];

layout(location=0) out vec2 baseColorUV;
layout(location=1) out vec2 normalMapUV;
layout(location=2) out vec2 metallicRoughnessUV;
layout(location=3) out vec2 occlusionUV;
layout(location=4) out vec2 emissiveUV;
layout(location=5) out mat3 vertTbn;
layout(location=8) out vec3 direction;

layout(set=0, binding=1) uniform UniformBufferObject {
  mat4 projection;
  mat4 inverseProjection;
  mat4 view;
  mat4 inverseView;
  vec3 lightDir;
  float time;
} globals;

// TODO: may be too big for inline block
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

layout(push_constant) uniform PushConstants {
  mat4 model;
} pushConstants;

void main() {
  vec3 cameraPos = globals.inverseView[3].xyz;

  vec4 worldPos = pushConstants.model * vec4(position, 1.0);

  direction = worldPos.xyz - cameraPos;

  gl_Position = globals.projection * globals.view * worldPos;

  baseColorUV = uvs[constants.baseTextureCoordinateIndex];
  normalMapUV = uvs[constants.normalMapTextureCoordinateIndex];
  metallicRoughnessUV = uvs[constants.metallicRoughnessTextureCoordinateIndex];
  occlusionUV = uvs[constants.occlusionTextureCoordinateIndex];
  emissiveUV = uvs[constants.emissiveTextureCoordinateIndex];

  vertTbn = mat3(pushConstants.model) * tbn;
}