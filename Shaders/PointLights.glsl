
#ifndef _POINTLIGHTS_
#define _POINTLIGHTS_

#include <Bindless/GlobalHeap.glsl>

struct PointLight {
  vec3 position;
  vec3 emission;
};

BUFFER_R(pointLights, PointLights{
  PointLight pointLightArr[];
});

UNIFORM_BUFFER(pointLightUniforms, PointLightUniforms{
  mat4 projection;
  mat4 inverseProjection;
  mat4 views[6];
  mat4 inverseViews[6];
  uint pointLightsBufferHandle;
  uint padding[3];
});

#endif // _POINTLIGHTS_