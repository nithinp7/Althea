#ifndef _SIMPLERTRESOURCES_
#define _SIMPLERTRESOURCES_

#define IS_SHADER
#include <../Include/Althea/Common/GltfCommon.h>

#include <Bindless/GlobalHeap.glsl>
#include <Global/GlobalResources.glsl>
#include <Global/GlobalUniforms.glsl>

layout(push_constant) uniform RtPush {
  uint globalResourcesHandle;
  uint globalUniformsHandle;
} push;

#define resources globalResources[push.globalResourcesHandle]
#define globals globalUniforms[push.globalUniformsHandle]

BUFFER_R(_primitiveConstants, _PrimitiveConstants {
  PrimitiveConstants c;
});
#define primitiveConstants(IDX) _primitiveConstants[IDX].c

// testing
struct RtPayload {
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