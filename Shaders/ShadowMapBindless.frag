layout(location=0) in vec3 worldPosCS;
layout(location=1) in vec2 baseColorUV;

layout(depth_any) out float gl_FragDepth;

#define PRIMITIVE_CONSTANTS_SET 1
// PRIMITIVE_CONSTANTS_BINDING must be defined during compilation
#include "PrimitiveConstants.glsl"

#define constants primitiveConstants[pushConstants.primId]

// TEXTURE_HEAP_BINDING and TEXTURE_HEAP_COUNT must be defined during compilation
layout(set=1, binding=TEXTURE_HEAP_BINDING) uniform sampler2D textureHeap[TEXTURE_HEAP_COUNT];

#define baseColorTexture textureHeap[5*pushConstants.primId+0]

layout(push_constant) uniform PushConstants {
  mat4 model;
  int primId;
} pushConstants;

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
