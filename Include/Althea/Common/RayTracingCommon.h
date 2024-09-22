#ifndef _RAYTRACINGCOMMON_
#define _RAYTRACINGCOMMON_

#include <../Include/Althea/Common/CommonTranslations.h>

struct RayTracingHandles {
  uint tlasHandle;
  
  uint targetImgHandle;
  uint targetTexHandle;

  uint padding;
};

#endif // _RAYTRACINGCOMMON_