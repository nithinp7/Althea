
#version 460 core

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
layout(location=8) out vec3 worldPosition;
layout(location=9) out vec3 direction;

#include "Bindless/GlobalHeap.glsl"
#include "Global/GlobalUniforms.glsl"
#include "Global/GlobalResources.glsl"
#include "PrimitiveResources.glsl"

layout(push_constant) uniform PushConstants {
  mat4 model;
  uint primId;
  uint globalResourcesHandle;
  uint globalUniformsHandle;
} pushConstants;

#define globals RESOURCE(globalUniforms, pushConstants.globalUniformsHandle)
#define resources RESOURCE(globalResources, pushConstants.globalResourcesHandle)

void main() {
  PrimitiveConstants constants = 
      RESOURCE(primitiveConstants, resources.primitiveConstantsBuffer)
        .primitiveConstantsArr[pushConstants.primId];

  vec4 worldPos4 = pushConstants.model * vec4(position, 1.0);
  worldPosition = worldPos4.xyz;

  gl_Position = globals.projection * globals.view * worldPos4;
  vec3 cameraPos = globals.inverseView[3].xyz;

  direction = worldPos4.xyz - cameraPos;

  baseColorUV = uvs[constants.baseTextureCoordinateIndex];
  normalMapUV = uvs[constants.normalMapTextureCoordinateIndex];
  metallicRoughnessUV = uvs[constants.metallicRoughnessTextureCoordinateIndex];
  occlusionUV = uvs[constants.occlusionTextureCoordinateIndex];
  emissiveUV = uvs[constants.emissiveTextureCoordinateIndex];

  vertTbn = mat3(pushConstants.model) * tbn;
}