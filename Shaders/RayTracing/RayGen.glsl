// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Ray generation shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadEXT vec4 payload;

layout(set=0, binding=0) uniform accelerationStructureEXT acc;
layout(set=0, binding=1) uniform rayParams
{
    uint sbtOffset;
    uint padding1;
    uint sbtStride;
    uint padding2;
    uint missIndex;
};

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 2
#include <GlobalUniforms.glsl>

// Output image
layout(set=0, binding=3) uniform writeonly image2D img;

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
  traceRayEXT(acc, gl_RayFlagsOpaqueEXT, 0xff, sbtOffset,
              sbtStride, missIndex, rayOrigin, 0.0,
              computeDir(gl_LaunchIDEXT, gl_LaunchSizeEXT),
              100.0f, 0 /* payload */);
  imageStore(img, ivec2(gl_LaunchIDEXT), payload);
}