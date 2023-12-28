#version 460 core

#extension GL_EXT_multiview : enable

layout(location=0) in vec3 position;
layout(location=1) in mat3 tbn;
// layout(location = 1) in vec3 tangent;
// layout(location = 2) in vec3 bitangent;
// layout(location = 3) in vec3 normal;
layout(location=4) in vec2 uvs[4];

layout(location=0) out vec3 worldPosCS;
layout(location=1) out vec2 baseColorUV;

#include <PointLights.glsl>

#include "Global/GlobalResources.glsl"
#include "PrimitiveConstants.glsl"

layout(push_constant) uniform PushConstants {
  mat4 model;
  uint primIdx;
  uint lightIdx;
  uint globalResourcesHandle;
  uint pointLightBufferHandle;
  uint pointLightConstantsHandle;
} pushConstants;

#define resources RESOURCE(globalResources,pushConstants.globalResourcesHandle)
#define lightConstants RESOURCE(pointLightConstants,pushConstants.pointLightConstantsHandle)

void main() {
  PointLight light = 
      RESOURCE(pointLights, pushConstants.pointLightBufferHandle)
        .pointLightArr[pushConstants.lightIdx];

  // TODO: Maybe don't need this level of indirection...
  PrimitiveConstants primConstants = 
      RESOURCE(primitiveConstants, resources.primitiveConstantsBuffer)
        .primitiveConstantsArr[pushConstants.primIdx];
  
  // Note the view matrix here is centered at the origin here for re-usability, we just subtract
  // the light position from the vertex position to compensate
  vec4 csPos = 
      lightConstants.views[gl_ViewIndex] * 
      pushConstants.model * 
      vec4(position - light.position, 1.0);
  gl_Position = lightConstants.projection * csPos;
  
  worldPosCS = csPos.xyz / csPos.w;
  baseColorUV = uvs[primConstants.baseTextureCoordinateIndex];
}