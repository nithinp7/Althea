#extension GL_EXT_multiview : enable

layout(location=0) in vec3 position;
layout(location=1) in mat3 tbn;
// layout(location = 1) in vec3 tangent;
// layout(location = 2) in vec3 bitangent;
// layout(location = 3) in vec3 normal;
layout(location=4) in vec2 uvs[4];

layout(location=0) out vec3 worldPosCS;
layout(location=1) out vec2 baseColorUV;

layout(set=0, binding=0) uniform UniformBufferObject {
  mat4 projection;
  mat4 inverseProjection;
  mat4 views[6];
  mat4 inverseViews[6];
} globals;

#define PRIMITIVE_CONSTANTS_SET 1
// PRIMITIVE_CONSTANTS_BINDING must be defined during compilation
#include "PrimitiveConstants.glsl"

#define constants primitiveConstants[pushConstants.primId]

layout(push_constant) uniform PushConstants {
  mat4 model;
  int primId;
} pushConstants;

void main() {
  vec4 csPos = globals.views[gl_ViewIndex] * pushConstants.model * vec4(position, 1.0);
  gl_Position = globals.projection * csPos;
  
  worldPosCS = csPos.xyz / csPos.w;
  baseColorUV = uvs[constants.baseTextureCoordinateIndex];
}