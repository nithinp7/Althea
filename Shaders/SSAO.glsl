
// Random number generator
// from ShaderToy https://www.shadertoy.com/view/4tXyWN
// TODO: Find better alternative??
uvec2 seed;
float rng() {
    seed += uvec2(1);
    uvec2 q = 1103515245U * ( (seed >> 1U) ^ (seed.yx) );
    uint  n = 1103515245U * ( (q.x) ^ (q.y >> 3U) );
    return float(n) * (1.0 / float(0xffffffffU));
}

// Useful functions for transforming directions
// TODO: check if the exact tangent, bitangent coords matter
void coordinateSystem(in vec3 v1, out vec3 v2, out vec3 v3) {
    if (abs(v1.x) > abs(v1.y))
            v2 = vec3(-v1.z, 0, v1.x) / sqrt(v1.x * v1.x + v1.z * v1.z);
        else
            v2 = vec3(0, v1.z, -v1.y) / sqrt(v1.y * v1.y + v1.z * v1.z);
        v3 = cross(v1, v2);
}

mat3 LocalToWorld(vec3 nor) {
  vec3 tan, bit;
  coordinateSystem(nor, tan, bit);
  return mat3(tan, bit, nor);
}

#define SSAO_RAY_COUNT 24
#define SSAO_RAYMARCH_STEPS 12
float computeSSAO(vec2 currentUV, vec3 worldPos, vec3 normal) {
  
  // vec4 projected = globals.projection * globals.view * vec4(rayDir, 0.0);
  // vec2 uvStep = (projected.xy / projected.w) / 128.0;
  float dx0 = 0.2;
  float dx = dx0;

  // currentUV += uvDir / 128.0;
  // return vec3(0.5) + 0.5 * rayDir;
  float ao = 0;

  for (int raySample = 0; raySample < SSAO_RAY_COUNT; ++raySample) {
    vec3 prevPos = worldPos;
    float prevProjection = 0.0;

    vec3 currentRayPos = worldPos;

    vec3 rayDir = normalize(vec3(rng(), rng(), rng()) + normal);

    vec3 perpRef = cross(rayDir, normal);
    perpRef = normalize(cross(perpRef, rayDir));

    for (int i = 0; i < SSAO_RAYMARCH_STEPS; ++i) {
      // currentUV += uvStep;
      currentRayPos += dx * rayDir;
      vec4 projected = globals.projection * globals.view * vec4(currentRayPos, 1.0);
      currentUV = 0.5 * projected.xy / projected.w + vec2(0.5);

      if (currentUV.x < 0.0 || currentUV.x > 1.0 || currentUV.y < 0.0 || currentUV.y > 1.0) {
        break;
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
      

      // float cosTheta = dot(dir, rayDir);
      // float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
      // if (acos(cosTheta) < 0.25) {

      if (currentProjection * prevProjection < 0.0 && worldStep <= 5 * dx && i > 0) {
        vec3 currentNormal = normalize(texture(gBufferNormal, currentUV, 0.0).xyz);
        // if (dot(currentNormal, rayDir) < 0) {
          ao += 1.0;
          break;
        // }
      }

      prevPos = currentPos;
      prevProjection = currentProjection;
    }
  }

  return 1.0 - ao / SSAO_RAY_COUNT;
}

// TODO: Use this function instead, currently it is still worse than the 
// above version somehow
float computeSSAO2(vec2 uvStart, vec3 worldPos, vec3 normal) {
  float ao = 0;

  mat3 localToWorld = LocalToWorld(normal);

  for (int raySample = 0; raySample < SSAO_RAY_COUNT; ++raySample) {
    vec3 xi = vec3(rng(), rng(), rng());
    vec3 rayDir = localToWorld * normalize(vec3(2.0 * xi.xy - vec2(1.0), xi.z));
    
    // Arbitrary
    vec3 endPos = worldPos + rayDir * 5.0;
    vec4 projectedEnd = globals.projection * globals.view * vec4(endPos, 1.0);
    vec2 uvEnd = 0.5 * projectedEnd.xy / projectedEnd.w + vec2(0.5);

    // Should probably create a local 2D tangent space along raydir
    vec3 perpRef = cross(rayDir, normal);
    perpRef = normalize(cross(perpRef, rayDir));
    
    vec3 prevPos = worldPos;
    float prevProjection = 0.0;

    for (int i = 0; i < SSAO_RAYMARCH_STEPS; ++i) {
      vec2 currentUV = mix(uvStart, uvEnd, i / float(SSAO_RAYMARCH_STEPS));
      if (currentUV.x < 0.0 || currentUV.x > 1.0 || currentUV.y < 0.0 || currentUV.y > 1.0) {
        break;
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
      
      if (currentProjection * prevProjection < 0.0 && worldStep <= 5.0 && i > 0) {
        // vec3 currentNormal = normalize(textureLod(gBufferNormal, currentUV, 0.0).xyz);
        // if (dot(currentNormal, rayDir) < 0) {
          ao += 1.0;
          break;
        // }
      }

      prevPos = currentPos;
      prevProjection = currentProjection;
    }
  }

  return 1.0 - ao / SSAO_RAY_COUNT;
}