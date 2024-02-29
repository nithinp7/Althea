
// Ray generation shader
#version 460 core

#define IS_RT_SHADER 1

#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"
#include "BRDF.glsl"

layout(location = 0) rayPayloadEXT PathTracePayload payload;

uvec2 seed;

bool findNearbySample(vec2 uv, vec3 p0, out GISample nearbySample) {
  vec2 nearbyUv = uv + 10.0 * randVec2(seed) / vec2(gl_LaunchSizeEXT);
  if (!isValidUV(nearbyUv)) {
    return false;
  }

  vec3 p1 = reconstructPosition(nearbyUv);

  float posDiscrepancy = length(p0 - p1);
  if (posDiscrepancy > 1.0)
  {
    return false;
  }

  ivec2 nearbyPx = getClosestPixel(nearbyUv);
  // Should fix these semantics... read idx here was the write idx in the direct sampling pass...
  uint readIdxOffset = 
      uint(giUniforms.writeIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint nearbyReservoirIdx = uint(nearbyPx.x * gl_LaunchSizeEXT.y + nearbyPx.y) + readIdxOffset;

  nearbySample = getReservoir(nearbyReservoirIdx).samples[0];
}

void main() {
  seed = uvec2(gl_LaunchIDEXT) * uvec2(giUniforms.framesSinceCameraMoved+1, giUniforms.framesSinceCameraMoved+2);

  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  vec2 scrUv = (vec2(pixelPos) + vec2(0.5)) / vec2(gl_LaunchSizeEXT);

  vec4 gb_albedo = texture(gBufferAlbedo, scrUv);
  if (gb_albedo.a == 0.0) {
    return;
  }

  vec3 p0 = reconstructPosition(scrUv);
  vec3 gb_normal = normalize(texture(gBufferNormal, scrUv).rgb);
  vec3 gb_metallicRoughnessOcclusion = texture(gBufferMetallicRoughnessOcclusion, scrUv).rgb;

  vec2 ndc = scrUv * 2.0 - 1.0;
  vec4 camPlanePos = globals.inverseProjection * vec4(ndc.x, ndc.y, 1, 1);
  vec3 wow = -(globals.inverseView*vec4(normalize(camPlanePos.xyz), 0)).xyz;

  uint readIdxOffset = 
      uint(giUniforms.writeIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint readReservoirIdx = 
      uint(gl_LaunchIDEXT.x * gl_LaunchSizeEXT.y + gl_LaunchIDEXT.y) + readIdxOffset;

  uint writeIdxOffset = 
      uint((giUniforms.writeIndex^1) * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint writeReservoirIdx = 
      uint(gl_LaunchIDEXT.x * gl_LaunchSizeEXT.y + gl_LaunchIDEXT.y) + writeIdxOffset;

  GISample curSample = sampleReservoirWeighted(readReservoirIdx, seed);
  float _;
  vec3 f0 = 
    evaluateMicrofacetBrdf(
        wow,
        curSample.wiw,
        gb_normal,
        gb_albedo.rgb,
        gb_metallicRoughnessOcclusion.x,
        gb_metallicRoughnessOcclusion.y,
        _);
  float l0 = max(dot(curSample.wiw, gb_normal), 0.0);
  vec3 irrSample0 = f0 * l0 * curSample.Li;
  float phat0 = length(irrSample0);

  for (int i = 0; i < 7; ++i) {
    GISample nearbySample;
    if (!findNearbySample(scrUv, p0, nearbySample))
      continue;

    vec3 f = 
      evaluateMicrofacetBrdf(
          wow,
          nearbySample.wiw,
          gb_normal,
          gb_albedo.rgb,
          gb_metallicRoughnessOcclusion.x,
          gb_metallicRoughnessOcclusion.y,
          _);
    vec3 irrSample = f * max(dot(nearbySample.wiw, gb_normal), 0.0) * nearbySample.Li;
    float phat = legnth(irrSample);
    float mi = 1.0 / 8.0;
    // float wi = mi * phat / nearbySample.pdf;

    nearbySample.pdf /= mi * phat;
    getReservoir(writeReservoirIdx).samples[i+1] = nearbySample;
  }

  // mis (TODO: use balance heuristic instead...)
  float m0 = 0.5;
  float m1 = 0.5;
  float w0 = m0 * phat0 / curSample.pdf;
  float w1 = m1 * phat1 / nearbySample.pdf;
  float wSum = w0 + w1;

  float r = rng(seed) * wSum;
  
   
  GISample outSample = 

  vec3 color = vec3(0.0);
  if (pdf > 0.01) 
  {
    float spatialW = 1.0 / pdf;

    uint sampleIdx = 0;
    vec3 Li = getReservoir(reservoirIdx).samples[sampleIdx].Li;
    vec3 wiw = getReservoir(reservoirIdx).samples[sampleIdx].wiw;
    float w = getReservoir(reservoirIdx).samples[sampleIdx].W;

    float wSum = spatialW + w;
    float r = rng(payload.seed) * wSum;
    
    if (r > w) {
      getReservoir(reservoirIdx).samples[sampleIdx].radiance = Li;
      getReservoir(reservoirIdx).samples[sampleIdx].dir = wiw;
      getReservoir(reservoirIdx).samples[sampleIdx].W = spatialW;
      w = spatialW;
    } else {
      Li = getReservoir(reservoirIdx).samples[sampleIdx].radiance;
      wiw = getReservoir(reservoirIdx).samples[sampleIdx].dir;
      f = 
        evaluateMicrofacetBrdf(
          wow,
          wiw,
          gb_normal,
          gb_albedo.rgb,
          gb_metallicRoughnessOcclusion.x,
          gb_metallicRoughnessOcclusion.y,
          pdf);
    }

    color = f * max(dot(wiw, gb_normal), 0.0) * Li * w;
  }

  validateColor(color);
  imageStore(colorTargetImg, pixelPos, vec4(color, 1.0));
}