#ifndef _RAYTRACINGCOMMON_
#define _RAYTRACINGCOMMON_

#include <../Include/Althea/Common/CommonTranslations.h>

struct RayTracingHandles {
  uint tlasHandle;
  
  uint targetImgHandle;
  uint targetTexHandle;

  uint padding;
};

struct RtPush {
  uint globalResourcesHandle;
  uint globalUniformsHandle;
  uint accumulatedFramesCount;
};

#endif // _RAYTRACINGCOMMON_