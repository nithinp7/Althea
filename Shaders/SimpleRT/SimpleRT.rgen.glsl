// Ray generation shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

#define IS_RT_SHADER
#include <SimpleRT/SimpleRTResources.glsl>

#include <Misc/Sampling.glsl>

layout(location = 0) rayPayloadEXT RtPayload payload;

vec3 computeDir(uvec3 launchID, uvec3 launchSize) {
  const vec2 pixelCenter = vec2(launchID.xy) + vec2(0.5);
	const vec2 inUV = pixelCenter/vec2(launchSize.xy);
	vec2 d = inUV * 2.0 - 1.0;

	vec4 origin = globals.inverseView * vec4(0,0,0,1);
	vec4 target = globals.inverseProjection * vec4(d.x, d.y, 1, 1) ;
	return (globals.inverseView*vec4(normalize(target.xyz), 0)).xyz;
}

uvec2 seed;
void main() {
  seed = uvec2(gl_LaunchIDEXT) * uvec2(globals.frameCount+1, globals.frameCount+2);

  vec3 rayOrigin = globals.inverseView[3].xyz;
  vec3 rayDir = computeDir(gl_LaunchIDEXT, gl_LaunchSizeEXT);
  payload.dir = rayDir;
  payload.xi = randVec2(seed);

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
  
  vec4 color = payload.color;

  imageStore(rtTargetImg, ivec2(gl_LaunchIDEXT), color);
}