#ifndef _GLOBALRESOURCES_
#define _GLOBALRESOURCES_

#include <Bindless/GlobalHeap.glsl>

struct GBufferHandles {
  uint positionHandle;
  uint normalHandle;
  uint albedoHandle;
  uint metallicRoughnessOcclusionHandle;
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
  uint primitiveConstantsBuffer;
});

#endif // _GLOBALRESOURCES_