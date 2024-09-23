#version 450

#define PI 3.14159265359

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D environmentMap; 
layout(set=0, binding=1) uniform sampler2D prefilteredMap; 
layout(set=0, binding=2) uniform sampler2D irradianceMap;
layout(set=0, binding=3) uniform sampler2D brdfLut;

#include <Global/GlobalResources.glsl>

// GBuffer textures
layout(set=1, binding=0) uniform sampler2D gBufferPosition;
layout(set=1, binding=1) uniform sampler2D gBufferNormal;
layout(set=1, binding=2) uniform sampler2D gBufferAlbedo;
layout(set=1, binding=3) uniform sampler2D gBufferMetallicRoughnessOcclusion;

// Prefiltered reflection buffer
layout(set=1, binding=4) uniform sampler2D reflectionBuffer;

#include <PBR/PBRMaterial.glsl>
#include <SSAO.glsl>

vec4 sampleReflection(float roughness) {
  return textureLod(reflectionBuffer, uv, 4.0 * roughness).rgba;
} 

vec3 sampleEnvMap(vec3 dir) {
  float yaw = atan(dir.z, dir.x);
  float pitch = -atan(dir.y, length(dir.xz));
  vec2 envMapUV = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, envMapUV, 0.0).rgb;
} 

void main() {
  seed = uvec2(gl_FragCoord.xy);

  vec4 position = texture(gBufferPosition, uv).rgba;
  if (position.a == 0.0) {
    // Nothing in the GBuffer, draw the environment map
    vec3 envMapSample = sampleEnvMap(direction);
#ifndef SKIP_TONEMAP
    envMapSample = vec3(1.0) - exp(-envMapSample * globals.exposure);
#endif
    outColor = vec4(envMapSample, 1.0);
    return;
  }

  vec3 normal = normalize(texture(gBufferNormal, uv).xyz);
  vec3 baseColor = texture(gBufferAlbedo, uv).rgb;
  vec3 metallicRoughnessOcclusion = //vec3(0.0, 0.2, 0.0);
      texture(gBufferMetallicRoughnessOcclusion, uv).rgb;

  vec3 reflectedDirection = reflect(normalize(direction), normal);
  vec4 reflectedColor = sampleReflection(metallicRoughnessOcclusion.y);
  vec4 envReflectedColor = vec4(sampleEnvMap(reflectedDirection, metallicRoughnessOcclusion.y), 1.0);
  if (reflectedColor.a < 0.01) {
    reflectedColor.rgb = envReflectedColor.rgb;
  } else {
    reflectedColor.rgb = 
        mix(envReflectedColor.rgb, reflectedColor.rgb / reflectedColor.a, reflectedColor.a);
  }

  vec3 irradianceColor = sampleIrrMap(normal);

  metallicRoughnessOcclusion.z = computeSSAO(uv, position.xyz, normal);

  vec3 material = 
      pbrMaterial(
        normalize(direction),
        globals.lightDir, 
        normal, 
        baseColor.rgb, 
        reflectedColor.rgb, 
        irradianceColor,
        metallicRoughnessOcclusion.x, 
        metallicRoughnessOcclusion.y, 
        metallicRoughnessOcclusion.z);

#ifndef SKIP_TONEMAP
  material = vec3(1.0) - exp(-material * globals.exposure);
#endif

  // outColor = vec4(0.5 * direction + vec3(0.5), 1.0);

  outColor = vec4(material, 1.0);
  // outColor = vec4(vec3(metallicRoughnessOcclusion.z), 1.0);
}