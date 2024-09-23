#version 460 core

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

layout(push_constant) uniform Push {
  uint globalResourcesHandle;
  uint globalUniformsHandle;
} push;

// TODO: Is there a better way to setup convenient access to global resources...
#define GLOBAL_RESOURCES_HANDLE push.globalResourcesHandle
#include <Global/GlobalResources.glsl>
#include <Global/GlobalUniforms.glsl>

#define globals globalUniforms[push.globalUniformsHandle]

#include <Misc/ReconstructPosition.glsl>
#include <Misc/Sampling.glsl>
#include <PathTracing/BRDF.glsl>

uvec2 seed;
void main() {
  seed = uvec2(gl_FragCoord.xy) * uvec2(globals.frameCount + 1, globals.frameCount + 2);
  vec2 xi = randVec2(seed);

  float d = texture(gBufferDepthA, uv).r;
  vec3 position = reconstructPosition(uv, d);
  vec3 normal = normalize(texture(gBufferNormal, uv).xyz);
  vec4 baseColor = texture(gBufferAlbedo, uv);
  vec3 metallicRoughnessOcclusion = texture(gBufferMetallicRoughnessOcclusion, uv).rgb;

  float metallic = metallicRoughnessOcclusion.x;
  float roughness = metallicRoughnessOcclusion.y;
  roughness = clamp(roughness, 0.01, 1.0);
  
  if (baseColor.a > 0.0) {
    vec3 wiw;
    float pdf;
    vec3 f = sampleMicrofacetBrdf(
      xi, -direction, normal,
      baseColor.rgb, metallic, roughness, 
      wiw, pdf);

    outColor = vec4(f * sampleEnvMap(wiw) / pdf, 1.0);
  } else {
    outColor = vec4(sampleEnvMap(direction), 1.0);
  }
}