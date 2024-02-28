
// Ray generation shader
#version 460 core

#define IS_RT_SHADER 1

#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"
#include "BRDF.glsl"

layout(location = 0) rayPayloadEXT PathTracePayload payload;

#define RAYMARCH_STEPS 32
vec4 raymarchGBuffer(vec2 currentUV, vec3 worldPos, vec3 normal, vec3 rayDir) {
  // Arbitrary
  vec3 endPos = worldPos + rayDir * 1000.0;
  vec4 projectedEnd = globals.projection * globals.view * vec4(endPos, 1.0);
  vec2 uvEnd = 0.5 * projectedEnd.xy / projectedEnd.w + vec2(0.5);

  // TODO: Handle degenerate case?
  vec2 uvStep = normalize(uvEnd - currentUV);
  float stepSize = 0.01;

  vec3 perpRef = cross(rayDir, normal);
  perpRef = normalize(cross(perpRef, rayDir));

  vec3 prevPos = worldPos;
  float prevProjection = 0.0;

  for (int i = 0; i < RAYMARCH_STEPS; ++i) {
    currentUV += uvStep * stepSize;
    if (currentUV.x < 0.0 || currentUV.x > 1.0 || currentUV.y < 0.0 || currentUV.y > 1.0) {
      return vec4(0.0);
    }

    // TODO: Check for invalid position
    
    vec3 currentPos = reconstructPosition(currentUV);
    vec3 dir = currentPos - worldPos;
    float currentProjection = dot(dir, perpRef);

    float dist = length(dir);
    dir = dir / dist;

    // TODO: interpolate between last two samples
    // Step between this and the previous sample
    float worldStep = length(currentPos - prevPos);
    
    if (currentProjection * prevProjection < 0.0 && worldStep <= 2.0 && i > 0) {
      vec3 currentNormal = normalize(textureLod(gBufferNormal, currentUV, 0.0).xyz);
      if (dot(currentNormal, rayDir) < 0) {
        return environmentLitSample(currentPos, currentUV, rayDir, currentNormal);
      }
    }

    prevPos = currentPos;
    prevProjection = currentProjection;
  }

  return vec4(0.0);//sampleEnvMap(rayDir), 1.0);
}

vec3 ssgi(out Li, out vec3 wiw, out float pdf) {
  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  vec2 jitter = vec2(0.5);//randVec2(payload.seed);
  vec2 scrUv = (vec2(pixelPos) + jitter) / vec2(gl_LaunchSizeEXT);

  vec2 ndc = scrUv * 2.0 - 1.0;
  vec4 camPlanePos = globals.inverseProjection * vec4(ndc.x, ndc.y, 1, 1);
  vec3 wow = -(globals.inverseView*vec4(normalize(camPlanePos.xyz), 0)).xyz;

  vec4 gb_albedo = texture(gBufferAlbedo, scrUv);
  if (gb_albedo.a == 0.0) {
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

    vec2 hitUv = projectToUV(payload.p)
    if (isValidUV(hitUv)) {

    }
    if (payload.emissive != vec3(0.0))
    {
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