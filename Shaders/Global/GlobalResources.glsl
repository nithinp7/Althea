#ifndef _GLOBALRESOURCES_
#define _GLOBALRESOURCES_

#define IS_SHADER
#include <Bindless/GlobalHeap.glsl>
#include <../Include/Althea/Common/RayTracingCommon.h>
#include <Misc/Constants.glsl>

struct GBufferHandles {
  uint depthAHandle;
  uint depthBHandle;
  uint normalHandle;
  uint albedoHandle;
  uint metallicRoughnessOcclusionHandle;
  uint stencilAHandle;
  uint stencilBHandle;
};

struct IBLHandles {
  uint environmentMapHandle;
  uint prefilteredMapHandle;
  uint irradianceMapHandle;
  uint brdfLutHandle;
};

BUFFER_R(globalResources, GlobalResources{
  GBufferHandles gBuffer;
  IBLHandles ibl;
  RayTracingHandles raytracing;
  uint shadowMapArray;
});

#endif // _GLOBALRESOURCES_

#ifdef GLOBAL_RESOURCES_HANDLE
#ifndef _GLOBALRESOURCESDECL_
#define _GLOBALRESOURCESDECL_

SAMPLER2D(_sampler2DHeap);

#define resources globalResources[GLOBAL_RESOURCES_HANDLE]

#define environmentMap _sampler2DHeap[resources.ibl.environmentMapHandle]
#define prefilteredMap _sampler2DHeap[resources.ibl.prefilteredMapHandle]
#define irradianceMap _sampler2DHeap[resources.ibl.irradianceMapHandle]
#define brdfLut _sampler2DHeap[resources.ibl.brdfLutHandle]

vec3 sampleEnvMap(vec3 dir) {
  float yaw = atan(dir.z, dir.x);
  float pitch = -atan(dir.y, length(dir.xz));
  vec2 envMapUV = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, envMapUV, 0.0).rgb;
} 

#define gBufferDepthA _sampler2DHeap[resources.gBuffer.depthAHandle]
#define gBufferDepthB _sampler2DHeap[resources.gBuffer.depthBHandle]
#define gBufferNormal _sampler2DHeap[resources.gBuffer.normalHandle]
#define gBufferAlbedo _sampler2DHeap[resources.gBuffer.albedoHandle]
#define gBufferMetallicRoughnessOcclusion _sampler2DHeap[resources.gBuffer.metallicRoughnessOcclusionHandle]

#endif // _GLOBALRESOURCESDECL_
#endif // GLOBAL_RESOURCES_HANDLE