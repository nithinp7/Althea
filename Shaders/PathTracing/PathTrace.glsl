
// Ray generation shader
#version 460 core

#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"
#include "BRDF.glsl"

layout(location = 0) rayPayloadEXT PathTracePayload payload;

bool isValidUV(vec2 uv) {
  return
      uv.x >= 0.0 && uv.x <= 1.0 && 
      uv.y >= 0.0 && uv.y <= 1.0;
}

vec3 computeDir(uvec3 launchID, uvec3 launchSize) {
  const vec2 jitteredPixel = vec2(launchID.xy) + randVec2(payload.seed);
	const vec2 inUV = jitteredPixel/vec2(launchSize.xy);
	vec2 d = inUV * 2.0 - 1.0;

	vec4 origin = globals.inverseView * vec4(0,0,0,1);
	vec4 target = globals.inverseProjection * vec4(d.x, d.y, 1, 1) ;
	return (globals.inverseView*vec4(normalize(target.xyz), 0)).xyz;
}

void updateReservoir(bool bCameraMoved, uint reservoirIdx, vec3 color, vec3 dir) {
  // Kick sample with lowest weight
  uint sampleIdx = getReservoir(reservoirIdx).sampleCount;
  getReservoir(reservoirIdx).sampleCount = (sampleIdx + 1) % 8;
  float w = getReservoir(reservoirIdx).samples[sampleIdx].W;
  if (!bCameraMoved)
  for (int i = 0; i < 8; ++i) {
    if (i == sampleIdx)
      continue;
    float w1 = getReservoir(reservoirIdx).samples[i].W;
    if (w1 < w) {
      sampleIdx = i;
      w = w1;
    }
  }

  getReservoir(reservoirIdx).samples[sampleIdx].radiance = color.rgb;
  getReservoir(reservoirIdx).samples[sampleIdx].dir = dir;

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

// TODO: Rename to something better
vec3 screenSpaceGI(vec3 pos, vec3 rayDir, vec3 N, float roughness, inout uvec2 seed) {
  vec2 prevUV = reprojectToPrevFrameUV(vec4(pos, 1.0));
  if (!isValidUV(prevUV)) {
    // TODO: Need to return invalid bool in this case
    return vec3(0.0);
  }

  vec2 pixelPosF = prevUV * vec2(gl_LaunchSizeEXT);
  ivec2 pixelPos = ivec2(pixelPosF);
  if (pixelPos.x < 0)
    pixelPos.x = 0;
  if (pixelPos.x >= gl_LaunchSizeEXT.x)
    pixelPos.x = int(gl_LaunchSizeEXT.x);
  if (pixelPos.y < 0)
    pixelPos.y = 0;
  if (pixelPos.y >= gl_LaunchSizeEXT.y)
    pixelPos.y = int(gl_LaunchSizeEXT.y);
    
  uint reservoirIdx = uint(pixelPos.x * gl_LaunchIDEXT.y + pixelPos.y);
  int sampleIdx = sampleReservoirIndexWeighted(reservoirIdx, seed);
  vec3 radiance = getReservoir(reservoirIdx).samples[sampleIdx].radiance;
  vec3 L = getReservoir(reservoirIdx).samples[sampleIdx].dir;
  float pdf = evaluateMicrofacetBrdfPdf(L, rayDir, N, roughness);
  if (pdf < 0.005)
    return vec3(0.0);
  
  return 0.01 * abs(dot(L, N)) * radiance / pdf;
}

bool validateColor(inout vec3 color) {
  if (color.x < 0.0 || color.y < 0.0 || color.z < 0.0) {
    color = vec3(1000.0, 0.0, 0.0);
    return false;
  }

  if (isnan(color.x) || isnan(color.y) || isnan(color.z)) {
    color = vec3(0.0, 1000.0, 0.0);
    return false;
  }  
  
  return true;
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
  vec3 firstBounceDir = vec3(0.0);

  vec3 throughput = vec3(1.0);
  vec3 color = vec3(0.0);
  for (int i = 0; i < 3; ++i)
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
      vec3 pos = payload.p.xyz / payload.p.w;

      // Compute prev screen-pos
      prevScrUv = reprojectToPrevFrameUV(payload.p); 

      depth = length(globals.inverseView[3].xyz - pos);
      expectedPrevDepth = length(globals.prevInverseView[3].xyz - pos);

      firstBounceRoughness = payload.roughness;
      firstBouncePos = pos;
      firstBounceDir = payload.wi;
    }

    color += throughput * payload.Lo;

    if (payload.throughput == vec3(0.0) || payload.p.w == 0.0)
    {
      break;
    }

    vec3 p = payload.p.xyz / payload.p.w;
    // If the current hit location is on-screen, sample the reservoir and early-out
    vec3 ssgiSample = screenSpaceGI(p, rayDir, payload.n, payload.roughness, payload.seed);
    if (ssgiSample != vec3(0.0)) {
      color += throughput * ssgiSample; 
      break;
    }

    // Set the next GI ray params
    rayDir = payload.wi;
    rayOrigin = payload.p.xyz / payload.p.w + 0.01 * rayDir;

    throughput *= payload.throughput;
  }

  mat4 viewDiff = globals.view - globals.prevView;
  bool bCameraMoved = length(viewDiff[0]) + length(viewDiff[1]) + length(viewDiff[2]) + length(viewDiff[3]) > 0.01;
  
#if 1
  uint reservoirIdx = pixelPos.x * gl_LaunchSizeEXT.y + pixelPos.y;
  updateReservoir(bCameraMoved, reservoirIdx, color.rgb, firstBounceDir);
  color.rgb = sampleReservoirWeighted(reservoirIdx, payload.seed); 
#endif

  vec4 prevColor = texture(prevColorTargetTx, prevScrUv);
  float prevDepth = texture(prevDepthTargetTx, prevScrUv).r;

  prevColor.a = min(prevColor.a, 50000.0);
  float depthDiscrepancy = abs(expectedPrevDepth - prevDepth);
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

// hacky temporal filtering...
  bool bColorValid = validateColor(color);
  vec4 blendedColor = vec4(color, 1.0);
#if 0  
  if (bColorValid && isValidUV(prevScrUv) && giUniforms.framesSinceCameraMoved > 0) {
    blendedColor = vec4(color + prevColor.rgb * prevColor.a, 1.0 + prevColor.a);
    blendedColor.rgb = blendedColor.rgb / blendedColor.a;
  }
#endif

  imageStore(colorTargetImg, pixelPos, blendedColor);
  imageStore(depthTargetImg, pixelPos, vec4(depth, 0.0, 0.0, 1.0));
}