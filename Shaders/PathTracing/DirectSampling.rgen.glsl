
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
      0.,
      dir,
      1000.0, 
      0 /* payload */);
  return payload.emissive;
}

void main() {
  payload.seed = uvec2(gl_LaunchIDEXT) * uvec2(giUniforms.framesSinceCameraMoved+1, giUniforms.framesSinceCameraMoved+2);

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

  float m = giUniforms.liveValues.temporalBlend;

  GISample newSample;
  newSample.Li = vec3(0.0);
  newSample.W = 0.0;

  float pdf0;
  vec3 f0 = 
      sampleMicrofacetBrdf(
        randVec2(payload.seed),
        wow,
        gb_normal,
        gb_albedo.rgb,
        gb_metallicRoughnessOcclusion.x,
        gb_metallicRoughnessOcclusion.y,
        newSample.wiw,
        pdf0);
  vec3 irrSample0 = vec3(0.0);
  if (pdf0 > 0.0001) {
    newSample.W = 1.0 / pdf0;
    newSample.Li = traceEnvMap(position, newSample.wiw);
    // TODO: div by pdf0??
    irrSample0 = f0 * max(dot(gb_normal, newSample.wiw), 0.0) * newSample.Li;
  }

  uint writeIndex = giUniforms.writeIndex;

  uint readIdxOffset = 
      // uint(writeIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
      uint((writeIndex^1) * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint writeIdxOffset = 
      uint(writeIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint writeReservoirIdx = 
      uint(gl_LaunchIDEXT.x * gl_LaunchSizeEXT.y + gl_LaunchIDEXT.y) + writeIdxOffset;

  // TODO: Do not want to clamp this if off-screen...
  vec2 prevUv = reprojectToPrevFrameUV(vec4(position, 1.0));
  ivec2 prevPx = getClosestPixel(prevUv);
  uint readReservoirIdx = 
      uint(prevPx.x * gl_LaunchSizeEXT.y + prevPx.y) + readIdxOffset;

  GISample temporalSample = getReservoir(readReservoirIdx).samples[0];
  vec3 irrSample1 = vec3(0.0);
  vec3 f1 = vec3(0.0);
  float pdf1 = 0.0;
  if (temporalSample.Li != vec3(0.0)) {
    pdf1 = 0.0;
    f1 = evaluateMicrofacetBrdf(
        wow,
        temporalSample.wiw,
        gb_normal,
        gb_albedo.rgb,
        gb_metallicRoughnessOcclusion.x,
        gb_metallicRoughnessOcclusion.y,
        pdf1);
    // TODO: div by pdf??
    if (pdf1 > 0.0001)
    {
      temporalSample.W = 1.0 / temporalSample.W / pdf1;
      irrSample1 = f1 * max(dot(gb_normal, temporalSample.wiw), 0.0) * temporalSample.Li;
    }
  }

  // pdf estimators
  float phat0 = length(irrSample0) + 0.0001;
  float phat1 = length(irrSample1) + 0.0001;
  
  // resampling weights
  // TODO: Multiply MIS weights here...
  float w0 = phat0 * newSample.W * m;
  float w1 = phat1 * temporalSample.W * (1.0 - m);

  float wSum = w0 + w1;
  float r = rng(payload.seed) * wSum;
  
  // vec3 color = vec3(w1);//vec3(0.01 * r);
  // vec3 color = vec3(abs(readReservoirIdx - writeReservoirIdx) - gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  // vec3 color = vec3(pdf1);
  vec3 color = vec3(0.0);
  // 0.5 * newSample.wiw + vec3(0.5);
  // vec3 color = vec3(length(temporalSample.wiw - newSample.wiw));
  // vec3 color = temporalSample.Li;//(phat1);//0.5 * temporalSample.wiw + vec3(0.5);
  if (r < w0)
  {
    newSample.W = wSum / phat0;
    // newSample.Li *= wSum / phat0;
    getReservoir(writeReservoirIdx).samples[0] = newSample;
    color = irrSample0 * newSample.W;
  } 
  else if (wSum > 0.0 && !isnan(wSum)) {
    temporalSample.W = wSum / phat1;
    // temporalSample.Li *= wSum / phat1;
    getReservoir(writeReservoirIdx).samples[0] = temporalSample;
    color = irrSample1 * temporalSample.W;
  } 
  //else {
  //   getReservoir(writeReservoirIdx).samples[0] = newSample;
  // }
  // else {
  //   temporalSample.W = 0.0;
  //   temporalSample.Li = vec3(0.0);
  //   getReservoir(writeReservoirIdx).samples[0] = temporalSample;
  // }

  validateColor(color);
  imageStore(colorTargetImg, pixelPos, vec4(color, 1.0));
}