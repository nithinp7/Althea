// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Ray generation shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

#define PI 3.14159265359
#define INV_PI 0.3183098861837697

#include <Misc/Sampling.glsl>

#include "PathTracePayload.glsl"
layout(location = 0) rayPayloadEXT PathTracePayload payload;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <GlobalUniforms.glsl>

layout(set=1, binding=0) uniform accelerationStructureEXT acc;

// Output image
layout(set=1, binding=1, rgba8) uniform image2D img;

layout(push_constant) uniform PathTracePushConstants {
  uint frameNumber; // frames since camera moved
} pushConstants;

vec3 computeDir(uvec3 launchID, uvec3 launchSize) {
  const vec2 pixelCenter = vec2(launchID.xy) + vec2(0.5);
	const vec2 inUV = pixelCenter/vec2(launchSize.xy);
	vec2 d = inUV * 2.0 - 1.0;

	vec4 origin = globals.inverseView * vec4(0,0,0,1);
	vec4 target = globals.inverseProjection * vec4(d.x, d.y, 1, 1) ;
	return (globals.inverseView*vec4(normalize(target.xyz), 0)).xyz;
}

void main() {
  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);

  vec3 prevColor = imageLoad(img, pixelPos).rgb;
  if (pushConstants.frameNumber == 0)
    prevColor = vec3(0.0);

  vec3 rayOrigin = globals.inverseView[3].xyz;
  vec3 rayDir = computeDir(gl_LaunchIDEXT, gl_LaunchSizeEXT);

  // "Naive" path-tracing
  float throughput = 1.0;
  vec3 color = vec3(0.0);
  for (int i = 0; i < 5; ++i)
  {
    payload.o = rayOrigin;
    payload.wo = -rayDir;
    traceRayEXT(
        acc, 
        gl_RayFlagsOpaqueEXT, 
        0xff, 
        0, // sbtOffset
        0, // sbtStride, 
        0, // missIndex
        rayOrigin, 
        0.0,
        rayDir,
        1000.0, 
        0 /* payload */);
    
    if (payload.Lo.x > 0.0 || payload.Lo.y > 0.0 || payload.Lo.z > 0.0)
    {
      // Ray hit a light, terminate immediately
      color = throughput * payload.Lo;
      break;
    }

    vec3 localRayDir = sampleHemisphereCosine();
    rayDir = normalize(LocalToWorld(payload.n) * localRayDir);
    rayOrigin = payload.p + 0.1 * rayDir;

    throughput *= INV_PI; // TODO: Actually use the pdf...
    // In this case the lambertian term and the cos in the 
    // pdf cancel out. 
  }

  vec3 blendedColor = (color + float(pushConstants.frameNumber) * prevColor) / float(pushConstants.frameNumber + 1);
  imageStore(img, pixelPos, vec4(blendedColor, 1.0));
}