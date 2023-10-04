// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Ray generation shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

#include "RayPayload.glsl"
layout(location = 0) rayPayloadEXT RayPayload payload;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <GlobalUniforms.glsl>

layout(set=1, binding=0) uniform accelerationStructureEXT acc;

// Output image
layout(set=1, binding=1) uniform writeonly image2D img;

vec3 computeDir(uvec3 launchID, uvec3 launchSize) {
  const vec2 pixelCenter = vec2(launchID.xy) + vec2(0.5);
	const vec2 inUV = pixelCenter/vec2(launchSize.xy);
	vec2 d = inUV * 2.0 - 1.0;

	vec4 origin = globals.inverseView * vec4(0,0,0,1);
	vec4 target = globals.inverseProjection * vec4(d.x, d.y, 1, 1) ;
	return (globals.inverseView*vec4(normalize(target.xyz), 0)).xyz;
}

void main() {
  vec3 rayOrigin = globals.inverseView[3].xyz;
  vec3 rayDir = computeDir(gl_LaunchIDEXT, gl_LaunchSizeEXT);
  payload.rayOriginIn = rayOrigin;
  payload.rayDirIn = rayDir;
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
  
  // vec3 dirCol = rayDir * 0.5 + vec3(0.5);
  vec4 color = payload.colorOut;//vec4(payload.rgb + dirCol, 1.0);

  imageStore(img, ivec2(gl_LaunchIDEXT), color);
}