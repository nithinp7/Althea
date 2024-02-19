
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

ivec2 getClosestPixel(vec2 uv) {
  vec2 pixelPosF = uv * vec2(gl_LaunchSizeEXT); 
  ivec2 pixelPos = ivec2(pixelPosF);
  if (pixelPos.x < 0)
    pixelPos.x = 0;
  if (pixelPos.x >= gl_LaunchSizeEXT.x)
    pixelPos.x = int(gl_LaunchSizeEXT.x-1);
  if (pixelPos.y < 0)
    pixelPos.y = 0;
  if (pixelPos.y >= gl_LaunchSizeEXT.y)
    pixelPos.y = int(gl_LaunchSizeEXT.y-1);
    
  return pixelPos;
}

vec3 computeDir(uvec3 launchID, uvec3 launchSize) {
  const vec2 jitteredPixel = vec2(launchID.xy) + randVec2(payload.seed);
	const vec2 inUV = jitteredPixel/vec2(launchSize.xy);
	vec2 d = inUV * 2.0 - 1.0;

	vec4 origin = globals.inverseView * vec4(0,0,0,1);
	vec4 target = globals.inverseProjection * vec4(d.x, d.y, 1, 1) ;
	return (globals.inverseView*vec4(normalize(target.xyz), 0)).xyz;
}

uint updateReservoir(vec2 scrUv, vec4 pos, vec3 dir, vec3 color) {
  vec2 prevScrUv = reprojectToPrevFrameUV(pos);
  bool bReprojectionValid = pos.w > 0.0 && isValidUV(prevScrUv);

  float expectedDepth = length(globals.prevInverseView[3].xyz - pos.xyz); 
  // float expectedDepth = length(globals.inverseView[3].xyz / globals.inverseView[3].w - pos.xyz/pos.w); 
  float prevDepth = texture(prevDepthTargetTx, prevScrUv).r;
  float depthDiscrepancy = abs(expectedDepth - prevDepth);
  bool bDepthsMatch = false;//log(depthDiscrepancy+1.0) < 1.;

  bReprojectionValid = bReprojectionValid && bDepthsMatch;

  // TODO: Depth-validate the reprojection...
  vec2 uv = bReprojectionValid ? prevScrUv : scrUv;

  // ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  ivec2 pixelPos = getClosestPixel(uv);
  
  // The reservoir heap is a ping-pong buffer. This offset can be added to a reservoir
  // index to access the corresponding reservoir in the back-buffer. 
  uint reservoirOffs = gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y;
  uint prevReservoirIdx = uint(pixelPos.x * gl_LaunchSizeEXT.y + pixelPos.y);
  uint reservoirIdx = uint(gl_LaunchIDEXT.x * gl_LaunchSizeEXT.y + gl_LaunchIDEXT.y);
  if (giUniforms.writeIndex == 0)
    prevReservoirIdx += reservoirOffs;
  else 
    reservoirIdx += reservoirOffs;
  
  uint sampleIdx = 0; 
  uint sampleCount = 1;
  if (bReprojectionValid)
  {
    getReservoir(reservoirIdx) = getReservoir(prevReservoirIdx);

    sampleCount = getReservoir(reservoirIdx).sampleCount;
    if (sampleCount < 8) {
      sampleIdx = sampleCount++;
    } else {
      // Reservoir full - kick sample with lowest weight
      float w = getReservoir(reservoirIdx).samples[sampleIdx].W;
      for (int i = 1; i < 8; ++i) {
        float w1 = getReservoir(reservoirIdx).samples[i].W;
        if (w1 < w) {
          sampleIdx = i;
          w = w1;
        }
      } 
    }
  }

  getReservoir(reservoirIdx).sampleCount = sampleCount;
  getReservoir(reservoirIdx).samples[sampleIdx].radiance = color.rgb;
  getReservoir(reservoirIdx).samples[sampleIdx].dir = dir;

  // update weights
  float wSum = 0.0;
  for (int i = 0; i < sampleCount; ++i) {
    float w = 
        i == sampleIdx ? 
          length(color.rgb) : 
          length(getReservoir(reservoirIdx).samples[i].radiance);
    wSum += w;
    getReservoir(reservoirIdx).samples[i].W = w;
  }

  getReservoir(reservoirIdx).wSum = wSum;

  return reservoirIdx;
}

// TODO: Rename to something better
vec3 screenSpaceGI(vec3 pos, vec3 rayDir, vec3 N, float roughness, inout uvec2 seed) {
  vec2 prevUV = reprojectToPrevFrameUV(vec4(pos, 1.0));
  if (!isValidUV(prevUV)) {
    // TODO: Need to return invalid bool in this case
    return vec3(0.0);
  }

  ivec2 pixelPos = getClosestPixel(prevUV);
  uint reservoirOffs = gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y;
  uint reservoirIdx = uint(pixelPos.x * gl_LaunchSizeEXT.y + pixelPos.y);
  if (giUniforms.writeIndex == 0)
    reservoirIdx += reservoirOffs;
  
  int sampleIdx = sampleReservoirIndexWeighted(reservoirIdx, seed);
  vec3 radiance = getReservoir(reservoirIdx).samples[sampleIdx].radiance;
  vec3 L = getReservoir(reservoirIdx).samples[sampleIdx].dir;
  float pdf = evaluateMicrofacetBrdfPdf(L, rayDir, N, roughness);
  if (pdf < 0.01)
    return vec3(0.0);
  
  return 0.025 * abs(dot(L, N)) * radiance / pdf;
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

  float firstBounceRoughness = -1.0;
  vec4 firstBouncePos = vec4(0.0);
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
      firstBounceRoughness = payload.roughness;
      firstBouncePos = payload.p;
      firstBounceDir = normalize(payload.wi);
    }

    color += throughput * payload.Lo;

    if (payload.throughput == vec3(0.0) || payload.p.w == 0.0)
    {
      break;
    }

    vec3 p = payload.p.xyz / payload.p.w;
#if 0
    // If the current hit location is on-screen, sample the reservoir and early-out
    vec3 ssgiSample = screenSpaceGI(p, rayDir, payload.n, payload.roughness, payload.seed);
    if (ssgiSample != vec3(0.0)) {
      color += throughput * ssgiSample; 
      break;
    }
#endif 

    // Set the next GI ray params
    rayDir = normalize(payload.wi);
    rayOrigin = payload.p.xyz / payload.p.w + 0.01 * rayDir;

    throughput *= payload.throughput;
  }

  // mat4 viewDiff = globals.view - globals.prevView;
  // bool bCameraMoved = length(viewDiff[0]) + length(viewDiff[1]) + length(viewDiff[2]) + length(viewDiff[3]) > 0.01;
  
  uint reservoirIdx = updateReservoir(scrUv, firstBouncePos, firstBounceDir, color.rgb);
  color.rgb = sampleReservoirWeighted(reservoirIdx, payload.seed); 

  validateColor(color);

  float depth = firstBouncePos.w == 0.0 ? 10000.0 : length(firstBouncePos.xyz / firstBouncePos.w - globals.inverseView[3].xyz);

  imageStore(colorTargetImg, pixelPos, vec4(color, 1.0));
  imageStore(depthTargetImg, pixelPos, vec4(depth, 0.0, 0.0, 1.0));
}