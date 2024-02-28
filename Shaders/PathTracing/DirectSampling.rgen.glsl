
// Ray generation shader
#version 460 core

#define IS_RT_SHADER 1

#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"
#include "BRDF.glsl"

layout(location = 0) rayPayloadEXT PathTracePayload payload;

vec3 directSample(out Li, out vec3 wiw, out float pdf) {
  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  vec2 scrUv = (vec2(pixelPos) + vec2(0.5)) / vec2(gl_LaunchSizeEXT);

  vec2 ndc = scrUv * 2.0 - 1.0;
  vec4 camPlanePos = globals.inverseProjection * vec4(ndc.x, ndc.y, 1, 1);
  vec3 wow = -(globals.inverseView*vec4(normalize(camPlanePos.xyz), 0)).xyz;

  vec4 gb_albedo = texture(gBufferAlbedo, scrUv);
  if (gb_albedo.a == 0.0) {
    // TODO: ???
    pdf = 0.0;
    return sampleEnvMap(-wow); 
  }
  
  vec3 position = reconstructPosition(scrUv);
  vec3 gb_normal = normalize(texture(gBufferNormal, scrUv).rgb);
  vec3 gb_metallicRoughnessOcclusion = texture(gBufferMetallicRoughnessOcclusion, scrUv).rgb;

  payload.seed = uvec2(gl_LaunchIDEXT) * uvec2(giUniforms.framesSinceCameraMoved+1, giUniforms.framesSinceCameraMoved+2);

  vec3 f = 
      sampleMicrofacetBrdf(
        randVec2(payload.seed),
        wow,
        gb_normal,
        gb_albedo.rgb,
        gb_metallicRoughnessOcclusion.x,
        gb_metallicRoughnessOcclusion.y,
        wiw,
        pdf);

  vec3 p = position + wiw * 0.1;

  Li = vec3(0.0);
  vec3 color = vec3(0.0);
  if (pdf > 0.0001)
  {
    payload.o = p;
    payload.wow = -wiw;
    traceRayEXT(
        acc, 
        gl_RayFlagsOpaqueEXT, 
        // gl_RayFlagsCullBackFacingTrianglesEXT,
        0xff, 
        0, // sbtOffset
        0, // sbtStride, 
        0, // missIndex
        p, 
        0.0,
        wiw,
        1000.0, 
        0 /* payload */);

    if (payload.emissive != vec3(0.0))
    {
      Li = payload.emissive;
      color = f * max(dot(wiw, gb_normal), 0.0) * payload.emissive / pdf;
    }
  }

  return color;
}

void main() {
  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  
  uint writeIdxOffset = 
      uint(giUniforms.writeIndex * gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
  uint reservoirIdx = 
      uint(gl_LaunchIDEXT.x * gl_LaunchSizeEXT.y + gl_LaunchIDEXT.y) + writeIdxOffset;

  vec3 Li;
  vec3 wiw;
  float pdf;
  vec3 color = directSample(Li, wiw, pdf);

  if (pdf > 0.01) 
  {
    getReservoir(reservoirIdx).samples[0].radiance = Li;
    getReservoir(reservoirIdx).samples[0].dir = wiw;
    getReservoir(reservoirIdx).samples[0].W = 1.0 / pdf;
    getReservoir(reservoirIdx).sampleCount = 1;
  }

  validateColor(color);
  imageStore(colorTargetImg, pixelPos, vec4(color, 1.0));
}