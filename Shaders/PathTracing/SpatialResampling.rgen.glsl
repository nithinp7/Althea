
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

#define KERNEL_RADIUS 100.0

bool findNearbySample(vec2 uv, vec3 p0, out float kernelPdf, out uint nearbyReservoirIdx) {
  vec2 relRadius = 2.0 * randVec2(seed) - vec2(1.0);
  kernelPdf = length(relRadius);
  kernelPdf = exp(-kernelPdf * kernelPdf);
  // kernelPdf = max(1.0 - length(relRadius), 0.0) + 0.01;
  // kernelPdf = exp(-kernelPdf);
  vec2 duv = KERNEL_RADIUS * relRadius / vec2(gl_LaunchSizeEXT);
  vec2 nearbyUv = uv + duv;
  if (!isValidUV(nearbyUv)) {
    return false;
  }

  vec3 p1 = reconstructPosition(nearbyUv);

  float posDiscrepancy = length(p0 - p1);
  if (posDiscrepancy > 10.0)
  {
    return false;
  }

  ivec2 nearbyPx = getClosestPixel(nearbyUv);
  // Should fix these semantics... read idx here was the write idx in the direct sampling pass...
  uint readIdxOffset = 
      uint(giUniforms.writeIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  nearbyReservoirIdx = uint(nearbyPx.x * gl_LaunchSizeEXT.y + nearbyPx.y) + readIdxOffset;

  return true;
}

struct SpatialSample {
  vec3 irradiance;
  float phat;
  float resamplingWeight;
  uint reservoirIdx;
};

void main() {
  seed = uvec2(gl_LaunchIDEXT) * uvec2(giUniforms.framesSinceCameraMoved+1, giUniforms.framesSinceCameraMoved+2);

  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  vec2 scrUv = (vec2(pixelPos) + vec2(0.5)) / vec2(gl_LaunchSizeEXT);

  vec2 ndc = scrUv * 2.0 - 1.0;
  vec4 camPlanePos = globals.inverseProjection * vec4(ndc.x, ndc.y, 1, 1);
  vec3 wow = -(globals.inverseView*vec4(normalize(camPlanePos.xyz), 0)).xyz;

  vec4 gb_albedo = texture(gBufferAlbedo, scrUv);
  if (gb_albedo.a == 0.0) {
    vec3 envSample = sampleEnvMap(-wow);
    imageStore(colorTargetImg, pixelPos, vec4(envSample, 1.0));
    return;
  }

  vec3 p0 = reconstructPosition(scrUv);
  vec3 gb_normal = normalize(texture(gBufferNormal, scrUv).rgb);
  vec3 gb_metallicRoughnessOcclusion = texture(gBufferMetallicRoughnessOcclusion, scrUv).rgb;

  uint readIdxOffset = 
      uint(giUniforms.writeIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint readReservoirIdx = 
      uint(gl_LaunchIDEXT.x * gl_LaunchSizeEXT.y + gl_LaunchIDEXT.y) + readIdxOffset;

  uint writeIdxOffset = 
      uint((giUniforms.writeIndex^1) * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint writeReservoirIdx = 
      uint(gl_LaunchIDEXT.x * gl_LaunchSizeEXT.y + gl_LaunchIDEXT.y) + writeIdxOffset;

#define SPATIAL_SAMPLE_COUNT 64
  SpatialSample spatialSamples[SPATIAL_SAMPLE_COUNT];
   
  // float mDenom = 1.0;
  float mDenom = 0.0;

  float wSum = 0.0;
  for (int i = 0; i < SPATIAL_SAMPLE_COUNT; ++i)
  {
    // TODO: Fix this terminology
    float kernelPdf = 1.0;
    if (i == 0)
    {
      spatialSamples[i].reservoirIdx = readReservoirIdx;
    }
    else 
    if (!findNearbySample(scrUv, p0, kernelPdf, spatialSamples[i].reservoirIdx)) {
      spatialSamples[i].resamplingWeight = 0.0;
      continue;
    }

    GISample s = getReservoir(spatialSamples[i].reservoirIdx).samples[0];

    float pdf;
    vec3 f = 
      evaluateMicrofacetBrdf(
          wow,
          s.wiw,
          gb_normal,
          gb_albedo.rgb,
          gb_metallicRoughnessOcclusion.x,
          gb_metallicRoughnessOcclusion.y,
          pdf);
    spatialSamples[i].irradiance = 
        f * max(dot(s.wiw, gb_normal), 0.0) * s.Li;
    // estimator for pdf
    spatialSamples[i].phat = length(spatialSamples[i].irradiance);
    // mis weight
    // float m = 1.0 / float(SPATIAL_SAMPLE_COUNT);
    // float m = length(s.Li);//i == 0 ? 10.0  : pdf;
    float m = 1.;//1.0 / s.W;
    mDenom += m;
    // float m = 1.0 / float(SPATIAL_SAMPLE_COUNT);
    // resampling weight
    spatialSamples[i].resamplingWeight = m * spatialSamples[i].phat * s.W;

    wSum += spatialSamples[i].resamplingWeight;
  }

  vec3 color = vec3(0.0);
  float x = rng(seed) * wSum;
  for (int i = 0; i < SPATIAL_SAMPLE_COUNT; ++i)
  {
    if (x < spatialSamples[i].resamplingWeight) {
      
      color = spatialSamples[i].irradiance * wSum / spatialSamples[i].phat / mDenom;
      GISample s = getReservoir(spatialSamples[i].reservoirIdx).samples[0]; 
      s.W = wSum / spatialSamples[i].phat / mDenom;
      // getReservoir(writeReservoirIdx).samples[0] = s; 
          
      break;
    } 

    x -= spatialSamples[i].resamplingWeight;
  }
  
  validateColor(color);
  // imageStore(colorTargetImg, pixelPos, vec4(color, 1.0));
}