#ifndef _SIMPLERTRESOURCES_
#define _SIMPLERTRESOURCES_

#define IS_SHADER
#include <Bindless/GlobalHeap.glsl>

#include <../Include/Althea/Common/RayTracingCommon.h>
layout(push_constant) uniform _RtPush {
  RtPush push;
};

#define GLOBAL_RESOURCES_HANDLE push.globalResourcesHandle
#include <Global/GlobalResources.glsl>

#include <Global/GlobalUniforms.glsl>
#define globals globalUniforms[push.globalUniformsHandle]

struct RtPayload {
  // input
  vec3 dir;
  vec2 xi;  

  // output
  vec4 color;
};

// TODO: use floating point textures
IMAGE2D_RW(imageHeap, rgba32f);
#define rtTargetImg imageHeap[resources.raytracing.targetImgHandle]

SAMPLER2D(sampler2DHeap);
#define rtTargetTex sampler2DHeap[resources.raytracing.targetTexHandle]

#ifdef IS_RT_SHADER
TLAS(tlasHeap);
#define acc tlasHeap[resources.raytracing.tlasHandle]
#endif // IS_RT_SHADER

#endif // _SIMPLERTRESOURCES_