
#version 450

#define PI 3.14159265359

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 reflectedColor;

layout(set=0, binding=0) uniform sampler2D environmentMap; 
layout(set=0, binding=1) uniform sampler2D prefilteredMap; 
layout(set=0, binding=2) uniform sampler2D irradianceMap;
layout(set=0, binding=3) uniform sampler2D brdfLut;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <GlobalUniforms.glsl>

#define POINT_LIGHTS_SET 0
#define POINT_LIGHTS_BINDING 5
#include <PointLights.glsl>

layout(set=0, binding=6) uniform samplerCubeArray shadowMapArray;

// GBuffer textures
layout(set=1, binding=0) uniform sampler2D gBufferPosition;
layout(set=1, binding=1) uniform sampler2D gBufferNormal;
layout(set=1, binding=2) uniform sampler2D gBufferAlbedo;
layout(set=1, binding=3) uniform sampler2D gBufferMetallicRoughnessOcclusion;

#include <PBR/PBRMaterial.glsl>

vec3 sampleEnvMap(vec3 dir) {
  float yaw = atan(dir.z, dir.x);
  float pitch = -atan(dir.y, length(dir.xz));
  vec2 envMapUV = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, envMapUV, 0.0).rgb;
} 

vec4 environmentLitSample(vec3 currentPos, vec2 currentUV, vec3 rayDir, vec3 normal) {
  vec3 baseColor = texture(gBufferAlbedo, currentUV).rgb;
  vec3 metallicRoughnessOcclusion = 
      texture(gBufferMetallicRoughnessOcclusion, currentUV).rgb;

  vec3 reflectedDirection = reflect(normalize(rayDir), normal);
  vec4 reflectedColor = vec4(sampleEnvMap(reflectedDirection, metallicRoughnessOcclusion.y), 1.0);
  vec3 irradianceColor = sampleIrrMap(normal);

  vec3 material = 
      pbrMaterial(
        currentPos,
        normalize(rayDir),
        normal, 
        baseColor.rgb, 
        reflectedColor.rgb, 
        irradianceColor,
        metallicRoughnessOcclusion.x, 
        metallicRoughnessOcclusion.y, 
        metallicRoughnessOcclusion.z);
  return vec4(material, 1.0);
}

void main() {
  vec4 position = texture(gBufferPosition, uv).rgba;
  if (position.a == 0.0) {
    // Nothing in the GBuffer, draw the environment map
    reflectedColor = vec4(0.0);
  }

  vec3 normal = normalize(texture(gBufferNormal, uv).xyz);
  vec3 reflectedDirection = reflect(normalize(direction), normal);

  reflectedColor = raymarchGBuffer(uv, position.xyz, normal, reflectedDirection);
}
