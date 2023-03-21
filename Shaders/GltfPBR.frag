
#version 450

#define PI 3.14159265359

layout(location=0) in vec2 baseColorUV;
layout(location=1) in vec2 normalMapUV;
layout(location=2) in vec2 metallicRoughnessUV;
layout(location=3) in vec2 occlusionUV;
layout(location=4) in vec2 emissiveUV;
layout(location=5) in mat3 fragTBN;
layout(location=8) in vec3 direction;
layout(location=9) in vec3 worldPosition;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D environmentMap; 
layout(set=0, binding=1) uniform sampler2D prefilteredMap; 
layout(set=0, binding=2) uniform sampler2D irradianceMap;
layout(set=0, binding=3) uniform sampler2D brdfLut;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include "GlobalUniforms.glsl"

// TODO: This is hacky, need to clean this up
#ifdef ENABLE_REFL_PROBES
layout(set=0, binding=5) uniform samplerCubeArray sceneCaptureTexArr;
#endif

layout(set=1, binding=0) uniform ConstantBufferObject {
  vec4 baseColorFactor;
  vec3 emissiveFactor;

  int baseTextureCoordinateIndex;
  int normalMapTextureCoordinateIndex;
  int metallicRoughnessTextureCoordinateIndex;
  int occlusionTextureCoordinateIndex;
  int emissiveTextureCoordinateIndex;

  float normalScale;
  float metallicFactor;
  float roughnessFactor;
  float occlusionStrength;

  float alphaCutoff;
} constants;

layout(set=1, binding=1) uniform sampler2D baseColorTexture;
layout(set=1, binding=2) uniform sampler2D normalMapTexture;
layout(set=1, binding=3) uniform sampler2D metallicRoughnessTexture;
layout(set=1, binding=4) uniform sampler2D occlusionTexture;
layout(set=1, binding=5) uniform sampler2D emissiveTexture;

#include "PBR/PBRMaterial.glsl"

void main() {
  vec3 normalMapSample = texture(normalMapTexture, normalMapUV).rgb;
  vec3 tangentSpaceNormal = 
      (2.0 * normalMapSample - 1.0) *
      vec3(constants.normalScale, constants.normalScale, 1.0);
  vec3 normal = normalize(fragTBN * tangentSpaceNormal);

  float ambientOcclusion = 
      texture(occlusionTexture, occlusionUV).r * constants.occlusionStrength;
  vec4 baseColor = texture(baseColorTexture, baseColorUV) * constants.baseColorFactor;

  vec2 metallicRoughness = 
      texture(metallicRoughnessTexture, metallicRoughnessUV).bg *
      vec2(constants.metallicFactor, constants.roughnessFactor);

  float metallic = metallicRoughness.x;
  float roughness = metallicRoughness.y;

  vec3 reflectedDirection = reflect(normalize(direction), normal);

vec3 debugTint = vec3(1.0);
#ifdef ENABLE_REFL_PROBES
  // TODO: hook up grid info, this is hackily hardcoded
  // probe grid is currently 10x4x4, where probe0 is at 
  // the origin and grid spacing is 10
  uvec3 ijk = uvec3(clamp(worldPosition / 10.0, vec3(0.0), vec3(9.0, 3.0, 3.0)));
  uint index = ijk.x * 4 * 4 + ijk.y * 4 + ijk.z;

  debugTint = vec3(ijk) / vec3(9.0, 3.0, 3.0);
  
  // Convert to OpenGL conventions.
  // Note: The captured cubemap is expected to intentionally have 
  // +X and -X switched to make this work - 
  // TODO: Look for a cleaner way...
  reflectedDirection.x *= -1.0; 
  vec3 reflectedColor = texture(sceneCaptureTexArr, vec4(reflectedDirection, index)).rgb;
#else
  vec3 reflectedColor = sampleEnvMap(reflectedDirection, roughness);
#endif

  vec3 irradianceColor = sampleIrrMap(normal);

  vec3 material = 
      pbrMaterial(
        normalize(direction),
        globals.lightDir, 
        normal, 
        baseColor.rgb, 
        reflectedColor, 
        irradianceColor,
        metallic, 
        roughness, 
        ambientOcclusion);

#ifndef SKIP_TONEMAP
  material = vec3(1.0) - exp(-material * globals.exposure);
#endif

  outColor = vec4(material, 1.0);
}
