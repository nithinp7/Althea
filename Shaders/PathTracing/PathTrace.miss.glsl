// Miss shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>
#include "PathTracePayload.glsl"

layout(location = 0) rayPayloadInEXT PathTracePayload payload;

#define PI 3.14159265359

vec3 sampleEnvMap(vec3 dir) {
  float yaw = atan(dir.z, dir.x);
  float pitch = -atan(dir.y, length(dir.xz));
  vec2 envMapUV = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, envMapUV, 0.0).rgb;
} 

void main() {
    payload.p = vec4(-payload.wow, 0.0);
    payload.wiw = vec3(0.0);
    payload.throughput = vec3(1.0);
    payload.Lo = 5.0 * sampleEnvMap(-payload.wow);
    payload.roughness = 1.0;
}