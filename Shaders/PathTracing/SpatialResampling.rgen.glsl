
// Ray generation shader
#version 460 core

#define IS_RT_SHADER 1

#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"
#include "BRDF.glsl"

layout(location = 0) rayPayloadEXT PathTracePayload payload;

vec3 spatialResample(out vec3 Li, out vec3 wiw, out float pdf) {
  pdf = 0.0;

  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  vec2 scrUv = (vec2(pixelPos) + vec2(0.5)) / vec2(gl_LaunchSizeEXT);
  vec2 nearbyUv = scrUv + 10.0 * randVec2(payload.seed) / vec2(gl_LaunchSizeEXT);
  if (!isValidUV(nearbyUv)) {
    return vec3(0.0);
  }

  vec4 gb_albedo = texture(gBufferAlbedo, scrUv);
  if (gb_albedo.a == 0.0) {
    return vec3(0.0);
  }

  vec3 p0 = reconstructPosition(scrUv);
  vec3 p1 = reconstructPosition(nearbyUv);

  float posDiscrepancy = length(p0 - p1);
  if (posDiscrepancy > 1.0)
  {
    return vec3(0.0);
  }

  ivec2 nearbyPx = getClosestPixel(nearbyUv);
  uint writeIdxOffset = 
      uint(giUniforms.writeIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint nearbyReservoirIdx = uint(nearbyPx.x * gl_LaunchSizeEXT.y + nearbyPx.y);

  wiw = getReservoir(nearbyReservoirIdx).samples[0].dir;
  Li = getReservoir(nearbyReservoirIdx).samples[0].radiance;

  vec2 ndc = scrUv * 2.0 - 1.0;
  vec4 camPlanePos = globals.inverseProjection * vec4(ndc.x, ndc.y, 1, 1);
  vec3 wow = -(globals.inverseView*vec4(normalize(camPlanePos.xyz), 0)).xyz;

  vec3 gb_normal = normalize(texture(gBufferNormal, scrUv).rgb);
  vec3 gb_metallicRoughnessOcclusion = texture(gBufferMetallicRoughnessOcclusion, scrUv).rgb;

  payload.seed = uvec2(gl_LaunchIDEXT) * uvec2(giUniforms.framesSinceCameraMoved+1, giUniforms.framesSinceCameraMoved+2);

  vec3 f = 
      evaluateMicrofacetBrdf(
        wow, 
        wiw, 
        gb_normal, 
        gb_albedo.rgb, 
        gb_metallicRoughnessOcclusion.x, 
        gb_metallicRoughnessOcclusion.y, 
        pdf)

  vec3 p = position + wiw * 0.1;

  vec3 color = vec3(0.0);
  if (pdf > 0.0001)
  {
    color = f * max(dot(wiw, gb_normal), 0.0) * Li / pdf;
  }

  return color;
}

void main() {
  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);

  vec3 Li;
  vec3 wiw;
  float pdf;
  vec3 color = spatialResample(Li, wiw, pdf);

  if (pdf > 0.01) 
  {
    getReservoir(reservoirIdx).samples[1].radiance = Li;
    getReservoir(reservoirIdx).samples[1].dir = wiw;
    getReservoir(reservoirIdx).samples[1].W = 1.0 / pdf;
    getReservoir(reservoirIdx).sampleCount = 2;
  }

  validateColor(color);
  imageStore(colorTargetImg, pixelPos, vec4(color, 1.0));
}