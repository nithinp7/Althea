
// Ray generation shader
#version 460 core

#extension GL_EXT_ray_tracing : enable

// #include "ImportanceSampling.glsl"
// #include "DirectLightSample.glsl"
#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"
layout(location = 0) rayPayloadEXT PathTracePayload payload;

vec3 computeDir(uvec3 launchID, uvec3 launchSize) {
  const vec2 jitteredPixel = vec2(launchID.xy) + randVec2(payload.seed);
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

  payload.seed = uvec2(gl_LaunchIDEXT) * uvec2(pushConstants.frameNumber+1, pushConstants.frameNumber+2);

  vec3 rayOrigin = globals.inverseView[3].xyz;
  vec3 rayDir = computeDir(gl_LaunchIDEXT, gl_LaunchSizeEXT);

  vec2 prevScrUv = vec2(-1.0);
  float expectedPrevDepth = -1.0;
  float depth = -1.0;
  float firstBounceRoughness = -1.0;
  vec3 firstBouncePos = vec3(0.0);
  vec3 dbgCol = normalize(rayDir) * 0.5 + vec3(0.5);

  vec3 throughput = vec3(1.0);
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

    if (i == 0) {
      // Compute prev screen-pos
      vec4 prevScrUvH = globals.projection * globals.prevView * payload.p;
      prevScrUv = prevScrUvH.xy / prevScrUvH.w * 0.5 + vec2(0.5);

      depth = length(globals.inverseView[3].xyz - payload.p.xyz / payload.p.w);
      expectedPrevDepth = length(globals.prevInverseView[3].xyz - payload.p.xyz / payload.p.w);

      firstBounceRoughness = payload.roughness;
      firstBouncePos = payload.p.xyz / payload.p.w;
    }

    color += throughput * payload.Lo;

    if (payload.throughput == vec3(0.0) || payload.p.w == 0.0)
    {
      break;
    }

  #if 1

  #endif

    // Set the next GI ray params
    rayDir = payload.wi;
    rayOrigin = payload.p.xyz / payload.p.w + 0.01 * rayDir;

    throughput *= payload.throughput;
  }

  vec4 blendedColor = vec4(0.0);

  vec4 prevColor = texture(prevImg, prevScrUv);
  float prevDepth = texture(prevDepthBuffer, prevScrUv).r;

  prevColor.a = min(prevColor.a, 50000.0);
  float depthDiscrepancy = abs(expectedPrevDepth - prevDepth);
  mat4 viewDiff = globals.view - globals.prevView;
  if (length(viewDiff[0]) + length(viewDiff[1]) + length(viewDiff[2]) + length(viewDiff[3]) > 0.01) {
    if (log(depthDiscrepancy + 1.0) > 0.5) {
    //if (depthDiscrepancy > 0.1) {
      prevColor.a = 0.0;
      // writeSpatialHash(firstBouncePos, color.rgb);
      // writeSpatialHash(firstBouncePos, dbgCol);
    } else {
      // If the first bounce is mostly diffuse we can keep more of the history than
      // the specular case. Fortunately, specular surfaces take fewer rays to converge
      // anyways.
      // TODO: Also consider modulating by log factor, and also by camera velocity
      // float trimmedHistory = mix(2.0, 500.0, firstBounceRoughness);
      float trimmedHistory = mix(2.0, 300.0, 2.0 * firstBounceRoughness / (1.0 * firstBounceRoughness + 1.0));
      prevColor.a = min(prevColor.a, trimmedHistory);
      // color += readSpatialHash(firstBouncePos);
    }
  }

  // prevColor.a = 0.0; // TODO: REmove

  if (pushConstants.frameNumber == 0 || 
      prevScrUv.x < 0.0 || prevScrUv.x > 1.0 || 
      prevScrUv.y < 0.0 || prevScrUv.y > 1.0) {
    blendedColor = vec4(color, 1.0);
  } else {
    // blendedColor = (color + float(pushConstants.frameNumber) * prevColor) / float(pushConstants.frameNumber + 1);
    blendedColor = vec4(color + prevColor.rgb * prevColor.a, 1.0 + prevColor.a);
    blendedColor.rgb = blendedColor.rgb / blendedColor.a;
  }

  if (color.x < 0.0 || color.y < 0.0 || color.z < 0.0)
    blendedColor = vec4(1000.0, 0.0, 0.0, 1.0);
  
  if (isnan(color.x) || isnan(color.y) || isnan(color.z))
    blendedColor = vec4(0.0, 1000.0, 0.0, 1.0);
    
  imageStore(img, pixelPos, blendedColor);
  imageStore(depthBuffer, pixelPos, vec4(depth, 0.0, 0.0, 1.0));
}