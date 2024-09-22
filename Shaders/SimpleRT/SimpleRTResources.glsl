#ifndef _SIMPLERTRESOURCES_
#define _SIMPLERTRESOURCES_

#define IS_SHADER
#include <../Include/Althea/Common/GltfCommon.h>

#include <Bindless/GlobalHeap.glsl>

layout(push_constant) uniform RtPush {
  uint globalResourcesHandle;
  uint globalUniformsHandle;
} push;

#define GLOBAL_RESOURCES_HANDLE push.globalResourcesHandle
#include <Global/GlobalResources.glsl>

#include <Global/GlobalUniforms.glsl>
#define globals globalUniforms[push.globalUniformsHandle]

SAMPLER2D(samplerHeap);
DECL_CONSTANTS(PrimitiveConstants, primitiveConstants);
DECL_BUFFER(R_PACKED, Vertex, primitiveVertices);
DECL_BUFFER(R_PACKED, uint, primitiveIndices);

struct RtPayload {
  // input
  vec3 dir;
  vec2 xi;  

  // output
  vec4 color;
};

IMAGE2D_W(imageHeap);
#define rtTargetImg imageHeap[resources.raytracing.targetImgHandle]

SAMPLER2D(sampler2DHeap);
#define rtTargetTex sampler2DHeap[resources.raytracing.targetTexHandle]

#ifdef IS_RT_SHADER
TLAS(tlasHeap);
#define acc tlasHeap[resources.raytracing.tlasHandle]
#endif // IS_RT_SHADER

#endif // _SIMPLERTRESOURCES_