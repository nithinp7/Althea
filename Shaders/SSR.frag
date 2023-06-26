
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
    
    vec3 currentPos = textureLod(gBufferPosition, currentUV, 0.0).xyz;
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
