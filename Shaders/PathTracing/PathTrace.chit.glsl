// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Closest hit shader
#version 460 core

#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"

layout(location = 0) rayPayloadInEXT PathTracePayload payload;
hitAttributeEXT vec2 attribs;
layout(location = 1) rayPayloadEXT PathTracePayload shadowRayPayload;

#include <PBR/PBRMaterial.glsl>

#include "DirectLightSample.glsl"

vec3 sampleEnvMap(vec3 dir) {
  float yaw = atan(dir.z, dir.x);
  float pitch = -atan(dir.y, length(dir.xz));
  vec2 envMapUV = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, envMapUV, 0.0).rgb;
} 

void main() {
    vec3 bc = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    uint idx0 = indexBufferHeap[gl_InstanceCustomIndexEXT].indices[3*gl_PrimitiveID+0];
    uint idx1 = indexBufferHeap[gl_InstanceCustomIndexEXT].indices[3*gl_PrimitiveID+1];
    uint idx2 = indexBufferHeap[gl_InstanceCustomIndexEXT].indices[3*gl_PrimitiveID+2];

    Vertex v0 = vertexBufferHeap[gl_InstanceCustomIndexEXT].vertices[idx0];
    Vertex v1 = vertexBufferHeap[gl_InstanceCustomIndexEXT].vertices[idx1];
    Vertex v2 = vertexBufferHeap[gl_InstanceCustomIndexEXT].vertices[idx2];

    PrimitiveConstants primInfo = primitiveConstants[gl_InstanceCustomIndexEXT];

    Vertex v;
    INTERPOLATE(position);
    INTERPOLATE(tangent);
    INTERPOLATE(bitangent);
    INTERPOLATE(normal);
    INTERPOLATE(uvs[0]);
    INTERPOLATE(uvs[1]);
    INTERPOLATE(uvs[2]);
    INTERPOLATE(uvs[3]);

    vec3 worldPos = gl_ObjectToWorldEXT * vec4(v.position, 1.0);
    vec4 baseColor = 
        texture(baseColorTexture, v.uvs[primInfo.baseTextureCoordinateIndex]) * primInfo.baseColorFactor;

    vec3 normalMapSample = texture(normalMapTexture, v.uvs[primInfo.baseTextureCoordinateIndex]).rgb;
    vec3 tangentSpaceNormal = 
        (2.0 * normalMapSample - 1.0) *
        vec3(primInfo.normalScale, primInfo.normalScale, 1.0);
    vec3 globalNormal = 
        normalize(gl_ObjectToWorldEXT *
        vec4(normalize(
          tangentSpaceNormal.x * v.tangent + 
          tangentSpaceNormal.y * v.bitangent + 
          tangentSpaceNormal.z * v.normal), 0.0));

    vec2 metallicRoughness = 
        texture(metallicRoughnessTexture, v.uvs[primInfo.metallicRoughnessTextureCoordinateIndex]).bg *
        vec2(primInfo.metallicFactor, primInfo.roughnessFactor);
    float ambientOcclusion = 
        texture(occlusionTexture, v.uvs[primInfo.occlusionTextureCoordinateIndex]).r * primInfo.occlusionStrength;

    // TODO: Support emissive objects
    // vec3 emissive = texture(emissiveTexture, v.uvs[primInfo.emissiveTextureCoordinateIndex]).rgb * primInfo.emissiveFactor.rgb;

    // if (emissive.x >  0.0 || emissive > 0.0 || emissive > 0.0)
    // {
    //   // TODO: Actually allow reflections off of lights
    //   payload.Lo = emissive;
    //   return;
    // }

    vec3 rayDir = -payload.wo;

   // vec3 irradianceColor = sampleIrrMap(globalNormal);
    // TODO: Might have to handle this logic in AnyHit shader?
    // if (GBuffer_Albedo.a < primInfo.alphaCutoff) {
    //   discard;
    // }

    // TODO: Change notation in Payload struct to use wow and wiw (world space)
    float pdf;
    vec3 f = 
        sampleMicrofacetBrdf(
          randVec2(payload.seed),
          worldPos,
          rayDir,
          globalNormal,
          baseColor.rgb,
          metallicRoughness.x,
          metallicRoughness.y,
          payload.wi,
          pdf);

    // TODO: Just for validation
    // if (f != vec3(0.0))
    //   f = evaluateBrdf(rayDir, payload.wi, globalNormal, baseColor.rgb, metallicRoughness.x, metallicRoughness.y);
    
    payload.p = vec4(worldPos, 1.0);
    payload.roughness = metallicRoughness.y;

    // TODO: Clamp low pdf samples (avoids fireflies...)
    if (f != vec3(0.0))
    {
      payload.throughput = f * abs(dot(payload.wi, globalNormal)) / pdf;
    } else {
      payload.throughput = vec3(0.0);
    }

    payload.Lo = vec3(0.0);
    // Direct illumination from point lights
    // payload.Lo = vec3(0.0); 
    // TODO: Check emissiveness first
    // Shadow ray to sample env map
#if 0
    {
      // payload.seed.x++;
      vec3 shadowRayDir = LocalToWorld(globalNormal) * sampleHemisphereCosine(payload.seed);
      // ;
      traceRayEXT(
        acc, 
        gl_RayFlagsOpaqueEXT, 
        0xff, 
        1, // sbtOffset
        0, // sbtStride, 
        0, // missIndex
        worldPos + shadowRayDir * 0.01, 
        0.0,
        shadowRayDir,
        1000.0, 
        1 /* payload */);

        // if (shadowRayPayload.p.x == 1.0)
        // {
        //   payload.Lo = shadowRayPayload.p.xyz;
        // }
        if (shadowRayPayload.p.w != 1.0) 
        {
          vec3 Li = sampleEnvMap(shadowRayDir);
          // vec3 Li = sampleIrrMap(shadowRayDir);
          vec3 f = 
              evaluateBrdf(
                rayDir, 
                shadowRayDir, 
                globalNormal, 
                baseColor.rgb, 
                metallicRoughness.x, 
                metallicRoughness.y);
          payload.Lo = f * Li / PI;
          // payload.throughput = vec3(1.0/0.0);// ???
          // payload.Lo = payload.throughput * f * Li / PI;
        }
    }
  #endif 
  
  #if 1
    //payload.Lo = vec3(0.0);
  #else
        illuminationFromPointLights(
          worldPos + payload.wi * 0.01,
          globalNormal,
          rayDir,
          baseColor.rgb,
          metallicRoughness.x,
          metallicRoughness.y,
          pdf * 0.0);
  #endif
}