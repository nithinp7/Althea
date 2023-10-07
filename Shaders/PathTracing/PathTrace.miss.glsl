// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Miss shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

#include "PathTracePayload.glsl"

layout(location = 0) rayPayloadInEXT PathTracePayload payload;

layout(set=0, binding=0) uniform sampler2D environmentMap; 

#define PI 3.14159265359

vec3 sampleEnvMap(vec3 dir) {
  float yaw = atan(dir.z, dir.x);
  float pitch = -atan(dir.y, length(dir.xz));
  vec2 envMapUV = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, envMapUV, 0.0).rgb;
} 

void main() {
    // TODO: homog position might be better...
    payload.p = vec3(0.0);
    payload.wi = vec3(0.0);
    payload.throughput = vec3(1.0);
    payload.Lo = 10.0 * sampleEnvMap(-payload.wo);
}