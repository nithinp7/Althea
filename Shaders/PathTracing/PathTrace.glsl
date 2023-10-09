
// Ray generation shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

#include "PathTracePayload.glsl"
layout(location = 0) rayPayloadEXT PathTracePayload payload;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <GlobalUniforms.glsl>

layout(set=1, binding=0) uniform accelerationStructureEXT acc;

// Output image
layout(set=1, binding=1) uniform sampler2D prevImg;
// Motion vectors
layout(set=1, binding=2, rgba32f) uniform image2D img;

layout(push_constant) uniform PathTracePushConstants {
  uint frameNumber; // frames since camera moved
} pushConstants;

// Random number generator and sample warping
// from ShaderToy https://www.shadertoy.com/view/4tXyWN
uvec2 seed;
float rng() {
    seed += uvec2(1);
    uvec2 q = 1103515245U * ( (seed >> 1U) ^ (seed.yx) );
    uint  n = 1103515245U * ( (q.x) ^ (q.y >> 3U) );
    return float(n) * (1.0 / float(0xffffffffU));
}

vec3 computeDir(uvec3 launchID, uvec3 launchSize) {
  const vec2 jitteredPixel = vec2(launchID.xy) + vec2(rng(), rng());
	const vec2 inUV = jitteredPixel/vec2(launchSize.xy);
	vec2 d = inUV * 2.0 - 1.0;

	vec4 origin = globals.inverseView * vec4(0,0,0,1);
	vec4 target = globals.inverseProjection * vec4(d.x, d.y, 1, 1) ;
	return (globals.inverseView*vec4(normalize(target.xyz), 0)).xyz;
}

void main() {
  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  // TODO: double check this works
  vec2 scrUv = vec2(pixelPos) / vec2(gl_LaunchSizeEXT);

  seed = uvec2(gl_LaunchIDEXT) * uvec2(pushConstants.frameNumber+1, pushConstants.frameNumber+2);

  vec3 rayOrigin = globals.inverseView[3].xyz;
  vec3 rayDir = computeDir(gl_LaunchIDEXT, gl_LaunchSizeEXT);

  vec2 prevScrUv = vec2(-1.0);

  vec3 throughput = vec3(1.0);
  vec3 color = vec3(0.0);
  for (int i = 0; i < 3; ++i)
  {
    payload.o = rayOrigin;
    payload.wo = -rayDir;
    payload.xi = vec2(rng(), rng());
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

    if (i == 0) {
      // Compute prev screen-pos
      vec4 prevScrUvH = globals.projection * globals.prevView * payload.p;
      prevScrUv = prevScrUvH.xy / prevScrUvH.w * 0.5 + vec2(0.5);
    }

    if (payload.Lo.x > 0.0 || payload.Lo.y > 0.0 || payload.Lo.z > 0.0)
    {
      // Ray hit a light, terminate immediately
      color = throughput * payload.Lo;
      break;
    }

    if (payload.p.w == 0.0) {
      break;
    }

    rayDir = payload.wi;
    rayOrigin = payload.p.xyz / payload.p.w + 0.01 * rayDir;;

    throughput *= payload.throughput;
  }

  vec3 blendedColor = vec3(0.0);

  vec3 prevColor = texture(prevImg, prevScrUv).rgb;
  if (pushConstants.frameNumber == 0 || 
      prevScrUv.x < 0.0 || prevScrUv.x > 1.0 || 
      prevScrUv.y < 0.0 || prevScrUv.y > 1.0) {
    blendedColor = color; // TODO: need to reset the frameNumber for just this pixel..
  } else {
    blendedColor = (color + float(pushConstants.frameNumber) * prevColor) / float(pushConstants.frameNumber + 1);
  }

  if (color.x < 0.0 || color.y < 0.0 || color.z < 0.0)
    blendedColor = vec3(1000.0, 0.0, 0.0);
  
  if (isnan(color.x) || isnan(color.y) || isnan(color.z))
    blendedColor = vec3(0.0, 1000.0, 0.0);
    
  imageStore(img, pixelPos, vec4(blendedColor, 1.0));
}