
#ifndef POINT_LIGHTS_SET
#define POINT_LIGHTS_SET 0
#endif

#ifndef POINT_LIGHTS_BINDING
#define POINT_LIGHTS_BINDING 0
#endif

struct PointLight {
  vec3 position;
  vec3 emission;
};

layout(std430, set=POINT_LIGHTS_SET, binding=POINT_LIGHTS_BINDING) readonly buffer POINT_LIGHTS {
  PointLight pointLightArr[];
};