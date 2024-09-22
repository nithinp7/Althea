#ifndef _GLOBALRESOURCES_
#define _GLOBALRESOURCES_

#include <Bindless/GlobalHeap.glsl>
#include <../Include/Althea/Common/RayTracingCommon.h>

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