
// Ray generation shader
#version 460 core

#define IS_RT_SHADER 1

#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"
#include "BRDF.glsl"

layout(location = 0) rayPayloadEXT PathTracePayload payload;

vec3 traceEnvMap(vec3 pos, vec3 dir) {
  payload.o = pos + 0.01 * dir;
  payload.wow = -dir;
  traceRayEXT(
      acc, 
      gl_RayFlagsOpaqueEXT, 
      // gl_RayFlagsCullBackFacingTrianglesEXT,
      0xff, 
      0, // sbtOffset
      0, // sbtStride, 
      0, // missIndex
      payload.o, 
      0.1,
      dir,
      1000.0, 
      0 /* payload */);
  return payload.emissive;
}

vec3 traceLight(vec3 pos, uint lightIdx, out vec3 dir) {
  vec3 color = getLightColor(lightIdx);

  // For now using the probes as lights
  vec3 lightPos = getProbe(lightIdx).position;
  dir = lightPos - pos;
  float dist = length(dir);
  dir /= dist;

  payload.o = pos + 0.01 * dir;
  payload.wow = -dir;
  traceRayEXT(
      acc, 
      gl_RayFlagsOpaqueEXT, 
      // gl_RayFlagsCullBackFacingTrianglesEXT,
      0xff, 
      0, // sbtOffset
      0, // sbtStride, 
      0, // missIndex
      payload.o, 
      0.1,
      dir,
      1000.0, 
      0 /* payload */);
  if (payload.p.w == 0.0 || length(payload.p.xyz - pos) > dist)
  {
    // light visible
    return getLightIntensity(dist) * color;
  } 

  // light shadowed
  return vec3(0.0);
}

void main() {
  payload.seed = uvec2(gl_LaunchIDEXT) * uvec2(giUniforms.frameNumber+1, giUniforms.frameNumber+2);

  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  vec2 scrUv = (vec2(pixelPos) + vec2(0.5)) / vec2(gl_LaunchSizeEXT);

  vec2 ndc = scrUv * 2.0 - 1.0;
  vec4 camPlanePos = globals.inverseProjection * vec4(ndc.x, ndc.y, 1, 1);
  vec3 wow = -(globals.inverseView*vec4(normalize(camPlanePos.xyz), 0)).xyz;

  vec4 gb_albedo = texture(gBufferAlbedo, scrUv);
  if (gb_albedo.a == 0.0) {
    return; 
  }
  
  vec3 position = reconstructPosition(scrUv);
  vec3 gb_normal = normalize(texture(gBufferNormal, scrUv).rgb);
  vec3 gb_metallicRoughnessOcclusion = texture(gBufferMetallicRoughnessOcclusion, scrUv).rgb;

  GISample newSample;
  newSample.Li = vec3(0.0);
  newSample.W = 0.0;

  float pdf0;
  vec3 f0;
  vec3 irrSample0 = vec3(0.0);

  if (bool(giUniforms.liveValues.flags & LEF_LIGHT_SAMPLING_MODE)) {
    // Light sampling mode
    uint probeCount = probesController.instanceCount;
    newSample.lightIdx = rngu(payload.seed) % probeCount;
    float pdfLight = 1.0 / float(probeCount);

    newSample.Li = traceLight(position, newSample.lightIdx, newSample.wiw);
    if (newSample.Li != vec3(0.0)) {
      f0 = evaluateMicrofacetBrdf(
        wow,
        newSample.wiw,
        gb_normal,
        gb_albedo.rgb,
        gb_metallicRoughnessOcclusion.x,
        gb_metallicRoughnessOcclusion.y,
        pdf0);

      if (pdf0 > 0.0001) 
      {
        newSample.W = float(probeCount); 
        // 1.0 / pdf0;//. / pdfLight;// / pdf0;
        irrSample0 = f0 * max(dot(gb_normal, newSample.wiw), 0.0) * newSample.Li;
      }
    }
  } else {
    // BRDF sampling + env-map mode
    f0 = sampleMicrofacetBrdf(
          randVec2(payload.seed),
          wow,
          gb_normal,
          gb_albedo.rgb,
          gb_metallicRoughnessOcclusion.x,
          gb_metallicRoughnessOcclusion.y,
          newSample.wiw,
          pdf0);
    if (pdf0 > 0.0001) {
      newSample.W = 1.0 / pdf0;
      newSample.Li = traceEnvMap(position, newSample.wiw);
      irrSample0 = f0 * max(dot(gb_normal, newSample.wiw), 0.0) * newSample.Li;
    }
  } 

  uint readIndex = 0;
  uint writeIndex = 1;

  uint readIdxOffset = 
      uint(readIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint writeIdxOffset = 
      uint(writeIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint writeReservoirIdx = 
      uint(gl_LaunchIDEXT.x * gl_LaunchSizeEXT.y + gl_LaunchIDEXT.y) + writeIdxOffset;

  // TODO: Do not want to clamp this if off-screen...
  vec2 prevUv = reprojectToPrevFrameUV(vec4(position, 1.0));
  ivec2 prevPx = getClosestPixel(prevUv);
  vec3 reprojectedPosition = reconstructPrevPosition(prevUv);
  uint readReservoirIdx = 
      uint(prevPx.x * gl_LaunchSizeEXT.y + prevPx.y) + readIdxOffset;

  GISample temporalSample = getReservoir(readReservoirIdx).s;
  vec3 irrSample1 = vec3(0.0);
  vec3 f1 = vec3(0.0);
  float pdf1 = 0.0;
  
  float depthDiscrepancy = length(reprojectedPosition - position);
  bool bReprojectionValid = isValidUV(prevUv) &&  depthDiscrepancy < getMaxDepthDiscrepancy();
  if (bool(giUniforms.liveValues.flags & LEF_LIGHT_SAMPLING_MODE) && 
      temporalSample.lightIdx >= probesController.instanceCount)
    bReprojectionValid = false;
  if (bReprojectionValid && temporalSample.Li != vec3(0.0)) {
    pdf1 = 0.0;
    f1 = evaluateMicrofacetBrdf(
        wow,
        temporalSample.wiw,
        gb_normal,
        gb_albedo.rgb,
        gb_metallicRoughnessOcclusion.x,
        gb_metallicRoughnessOcclusion.y,
        pdf1);
    if (pdf1 > 0.0001)
    {
      // temporalSample.W = 1.0 / temporalSample.W / pdf1;
      irrSample1 = f1 * max(dot(gb_normal, temporalSample.wiw), 0.0) * temporalSample.Li;
    } else {
      temporalSample.W = 0.0;
    }
  } else if (!bReprojectionValid) {
    temporalSample.W = 0.0;
  }

  // pdf estimators
  float phat0 = length(irrSample0);// + 0.0001;
  float phat1 = length(irrSample1);// + 0.0001;

  float m0 = giUniforms.liveValues.temporalBlend;
  float m1 = 1.0 - m0;
  m0 *= phat0;
  m1 *= phat1;
  
  float msum = m0 + m1;
  m0 /= msum;
  m1 /= msum;
  
  // resampling weights
  float w0 = phat0 * newSample.W * m0;
  float w1 = phat1 * temporalSample.W * m1;

  float wSum = w0 + w1;
  float r = rng(payload.seed) * wSum;
  
  vec3 color = vec3(0.0);
  // if (w1 > w0) {
  if (r < w1) {
    temporalSample.W = clamp(wSum / phat1, 0.0, MAX_W);
    getReservoir(writeReservoirIdx).s = temporalSample;
    color = irrSample1 * temporalSample.W;
  } else
  {
    newSample.W = clamp(wSum / phat0, 0.0, MAX_W);
    getReservoir(writeReservoirIdx).s = newSample;
    color = irrSample0 * newSample.W;
  } 

  validateColor(color);
  // imageStore(colorTargetImg, pixelPos, vec4(color, 1.0));
}