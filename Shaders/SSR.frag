
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

#define RAYMARCH_STEPS 32
vec4 raymarchGBuffer(vec2 currentUV, vec3 worldPos, vec3 normal, vec3 rayDir) {
  vec3 perpRef = cross(rayDir, normal);
  perpRef = normalize(cross(perpRef, rayDir));

  // vec4 projected = globals.projection * globals.view * vec4(rayDir, 0.0);
  // vec2 uvStep = (projected.xy / projected.w) / 128.0;
  float dx0 = 0.25;
  float dx = dx0;

  // currentUV += uvDir / 128.0;
  // return vec3(0.5) + 0.5 * rayDir;

  vec3 prevPos = worldPos;
  float prevProjection = 0.0;

  vec3 currentRayPos = worldPos;

  for (int i = 0; i < RAYMARCH_STEPS; ++i) {
    // currentUV += uvStep;
    currentRayPos += dx * rayDir;
    vec4 projected = globals.projection * globals.view * vec4(currentRayPos, 1.0);
    currentUV = 0.5 * projected.xy / projected.w + vec2(0.5);

    if (currentUV.x < 0.0 || currentUV.x > 1.0 || currentUV.y < 0.0 || currentUV.y > 1.0) {
      continue;
    }

    // TODO: Check for invalid position
    
    vec3 currentPos = textureLod(gBufferPosition, currentUV, 0.0).xyz;
    vec3 dir = currentPos - worldPos;
    float currentProjection = dot(dir, perpRef);

    float dist = length(dir);
    dir = dir / dist;

    dx = 5.0 * (1.0 + dx0 - 1.0 / (1.0 + dist));

    // TODO: interpolate between last two samples
    // Step between this and the previous sample
    float worldStep = length(currentPos - prevPos);
    

    // float cosTheta = dot(dir, rayDir);
    // float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    // if (acos(cosTheta) < 0.25) {

    if (currentProjection * prevProjection < 0.0 && worldStep <= 5 * dx && i > 0) {
      vec3 currentNormal = normalize(texture(gBufferNormal, currentUV, 0.0).xyz);
      if (dot(currentNormal, rayDir) < 0) {
        // TODO: direct lighting in reflection
        return vec4(texture(gBufferAlbedo, currentUV, 0.0).rgb, 1.0);
          //  return vec3(float(i) / RAYMARCH_STEPS, 0.0, 0.0);
      }
    }

    prevPos = currentPos;
    prevProjection = currentProjection;
  }

  return vec4(sampleEnvMap(rayDir), 1.0);
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
