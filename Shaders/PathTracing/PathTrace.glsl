
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
  vec2 scrUv = vec2(pixelPos) / vec2(gl_LaunchSizeEXT);

  payload.seed = uvec2(gl_LaunchIDEXT) * uvec2(giUniforms.framesSinceCameraMoved+1, giUniforms.framesSinceCameraMoved+2);

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

    // Set the next GI ray params
    rayDir = payload.wi;
    rayOrigin = payload.p.xyz / payload.p.w + 0.01 * rayDir;

    throughput *= payload.throughput;
  }

  vec4 blendedColor = vec4(0.0);

  vec4 prevColor = texture(prevColorTargetTx, prevScrUv);
  float prevDepth = texture(prevDepthTargetTx, prevScrUv).r;

  prevColor.a = min(prevColor.a, 50000.0);
  float depthDiscrepancy = abs(expectedPrevDepth - prevDepth);
  mat4 viewDiff = globals.view - globals.prevView;
  bool bCameraMoved = length(viewDiff[0]) + length(viewDiff[1]) + length(viewDiff[2]) + length(viewDiff[3]) > 0.01;
  if (bCameraMoved) {
    if (log(depthDiscrepancy + 1.0) > 0.5) {
    //if (depthDiscrepancy > 0.1) {
      prevColor.a = 0.0;
    } else {
      // If the first bounce is mostly diffuse we can keep more of the history than
      // the specular case. Fortunately, specular surfaces take fewer rays to converge
      // anyways.
      // TODO: Also consider modulating by log factor, and also by camera velocity
      // float trimmedHistory = mix(2.0, 500.0, firstBounceRoughness);
      float trimmedHistory = mix(2.0, 300.0, 2.0 * firstBounceRoughness / (1.0 * firstBounceRoughness + 1.0));
      prevColor.a = min(prevColor.a, trimmedHistory);
    }
  }

  if (giUniforms.framesSinceCameraMoved == 0 || 
      prevScrUv.x < 0.0 || prevScrUv.x > 1.0 || 
      prevScrUv.y < 0.0 || prevScrUv.y > 1.0) {
    blendedColor = vec4(color, 1.0);
  } else {
    // blendedColor = (color + float(giUniforms.framesSinceCameraMoved) * prevColor) / float(giUniforms.framesSinceCameraMoved + 1);
    blendedColor = vec4(color + prevColor.rgb * prevColor.a, 1.0 + prevColor.a);
    blendedColor.rgb = blendedColor.rgb / blendedColor.a;
  }

  if (color.x < 0.0 || color.y < 0.0 || color.z < 0.0)
    blendedColor = vec4(1000.0, 0.0, 0.0, 1.0);
  
  if (isnan(color.x) || isnan(color.y) || isnan(color.z))
    blendedColor = vec4(0.0, 1000.0, 0.0, 1.0);
    
  imageStore(colorTargetImg, pixelPos, blendedColor);
  imageStore(depthTargetImg, pixelPos, vec4(depth, 0.0, 0.0, 1.0));

  uint reservoirIdx = pixelPos.x * gl_LaunchSizeEXT.y + pixelPos.y;

  // Kick sample with lowest weight
  uint sampleIdx = getReservoir(reservoirIdx).sampleCount;
  getReservoir(reservoirIdx).sampleCount = (sampleIdx + 1) % 8;
  float w = getReservoir(reservoirIdx).samples[sampleIdx].W;
  for (int i = 0; i < 8; ++i) {
    if (i == sampleIdx)
      continue;
    float w1 = getReservoir(reservoirIdx).samples[i].W;
    if (w1 < w) {
      sampleIdx = i;
      w = w1;
    }
  }

  // getReservoir(reservoirIdx).sampleCount = (sampleIdx + 1) % 8;
  getReservoir(reservoirIdx).samples[sampleIdx].radiance = color.rgb;
  
  if (bCameraMoved) 
  {
    // TODO: Find better approach
    for (int i = 0; i < 8; ++i) {
      getReservoir(reservoirIdx).samples[i].radiance = 8.0 * color.rgb;
    }
  }

  // update weights
  float wSum = 0.0;
  for (int i = 0; i < 8; ++i) {
    float w = 
        i == sampleIdx ? 
          length(color.rgb) : 
          length(getReservoir(reservoirIdx).samples[i].radiance);
    wSum += w;
    getReservoir(reservoirIdx).samples[i].W = w;
  }

  getReservoir(reservoirIdx).wSum = wSum;
}