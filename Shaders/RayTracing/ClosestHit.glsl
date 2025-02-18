// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Closest hit shader
#version 460 core

#extension GL_EXT_ray_tracing : enable
// TODO: enable this ext in vulkan
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable

#include "RayPayload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
hitAttributeEXT vec2 attribs;

layout(set=0, binding=0) uniform sampler2D environmentMap; 
layout(set=0, binding=1) uniform sampler2D prefilteredMap; 
layout(set=0, binding=2) uniform sampler2D irradianceMap;
layout(set=0, binding=3) uniform sampler2D brdfLut;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <Global/GlobalUniforms.glsl>

#define POINT_LIGHTS_SET 0
#define POINT_LIGHTS_BINDING 5
#include <PointLights.glsl>

layout(set=0, binding=6) uniform samplerCubeArray shadowMapArray;

#ifndef TEXTURE_HEAP_COUNT
#define TEXTURE_HEAP_COUNT 1
#endif

layout(set=0, binding=7) uniform sampler2D textureHeap[TEXTURE_HEAP_COUNT];

#define PRIMITIVE_CONSTANTS_SET 0
#define PRIMITIVE_CONSTANTS_BINDING 8
#include <PrimitiveResources.glsl>

struct Vertex {
  vec3 position;
  vec3 tangent;
  vec3 bitangent;
  vec3 normal;
  vec2 uvs[4];
};

#extension GL_EXT_scalar_block_layout : enable
layout(scalar, set=0, binding=9) readonly buffer VERTEX_BUFFER_HEAP { Vertex vertices[]; } vertexBufferHeap[VERTEX_BUFFER_HEAP_COUNT];
layout(set=0, binding=10) readonly buffer INDEX_BUFFER_HEAP { uint indices[]; } indexBufferHeap[INDEX_BUFFER_HEAP_COUNT];

#include <PBR/PBRMaterial.glsl>

#define baseColorTexture textureHeap[5*gl_InstanceCustomIndexEXT+0]
#define normalMapTexture textureHeap[5*gl_InstanceCustomIndexEXT+1]
#define metallicRoughnessTexture textureHeap[5*gl_InstanceCustomIndexEXT+2]
#define occlusionTexture textureHeap[5*gl_InstanceCustomIndexEXT+3]
#define emissiveTexture textureHeap[5*gl_InstanceCustomIndexEXT+4]

#define INTERPOLATE(member)(v.member=v0.member*bc.x+v1.member*bc.y+v2.member*bc.z)

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

    // TODO: Model matrix heap needed to reconstruct world position?? Is it somewhere accessible from accel str
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

    vec3 reflectedDirection = reflect(payload.rayDirIn, globalNormal);
    vec4 reflectedColor = vec4(sampleEnvMap(reflectedDirection, metallicRoughness.y), 1.0);
    
    vec3 irradianceColor = sampleIrrMap(globalNormal);
    // TODO: Might have to handle this logic in AnyHit shader?
    // if (GBuffer_Albedo.a < primInfo.alphaCutoff) {
    //   discard;
    // }

    
    vec3 material = 
        pbrMaterial(
          worldPos,
          payload.rayDirIn,
          globalNormal, 
          baseColor.rgb, 
          vec3(0.0),//reflectedColor.rgb, 
          irradianceColor,
          metallicRoughness.x, 
          metallicRoughness.y, 
          ambientOcclusion);

  // TODO: Why is gl_ObjectToWorldEXT identity???
    // material = vec3(1.0, 0.0, 0.0);
    // material.xy = vec3(metallicRoughness, ambientOcclusion);
    // material = vec3(log(length(gl_ObjectToWorldEXT[3]) / 10.0));
    // material = vec3(determinant(mat3(gl_ObjectToWorldEXT)));//vec3(log(length(gl_ObjectToWorldEXT[3]) / 10.0));

#ifndef SKIP_TONEMAP
    material = vec3(1.0) - exp(-material * globals.exposure);
#endif

    // vec3 color = vec3(float(gl_PrimitiveID % 1000) / 1000.0);//texture(textureHeap[5*gl_InstanceCustomIndexEXT], uv).rgb;
    // vec3 color = texture(textureHeap[5*gl_InstanceCustomIndexEXT], uv).rgb;

    payload.colorOut = vec4(material, 1.0);
}