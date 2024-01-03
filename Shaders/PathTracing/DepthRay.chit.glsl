// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Closest hit shader
#version 460 core

#extension GL_EXT_ray_tracing : enable
// TODO: enable this ext in vulkan
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable

#include "PathTracePayload.glsl"

layout(location = 0) rayPayloadInEXT PathTracePayload payload;
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

layout(set=0, binding=7) uniform sampler2D textureHeap[];

#define PRIMITIVE_CONSTANTS_SET 0
#define PRIMITIVE_CONSTANTS_BINDING 8
#include <PrimitiveConstants.glsl>

struct Vertex {
  vec3 position;
  vec3 tangent;
  vec3 bitangent;
  vec3 normal;
  vec2 uvs[4];
};

#extension GL_EXT_scalar_block_layout : enable
layout(scalar, set=0, binding=9) readonly buffer VERTEX_BUFFER_HEAP { Vertex vertices[]; } vertexBufferHeap[];
layout(set=0, binding=10) readonly buffer INDEX_BUFFER_HEAP { uint indices[]; } indexBufferHeap[];

#include <PBR/PBRMaterial.glsl>

#include "DirectLightSample.glsl"

#define baseColorTexture textureHeap[5*gl_InstanceCustomIndexEXT+0]
#define normalMapTexture textureHeap[5*gl_InstanceCustomIndexEXT+1]
#define metallicRoughnessTexture textureHeap[5*gl_InstanceCustomIndexEXT+2]
#define occlusionTexture textureHeap[5*gl_InstanceCustomIndexEXT+3]
#define emissiveTexture textureHeap[5*gl_InstanceCustomIndexEXT+4]

#define INTERPOLATE(member)(v.member=v0.member*bc.x+v1.member*bc.y+v2.member*bc.z)

void main() {
  payload.p = vec4(1.0, 0.0, 0.0, 1.0);
    // vec3 bc = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // uint idx0 = indexBufferHeap[gl_InstanceCustomIndexEXT].indices[3*gl_PrimitiveID+0];
    // uint idx1 = indexBufferHeap[gl_InstanceCustomIndexEXT].indices[3*gl_PrimitiveID+1];
    // uint idx2 = indexBufferHeap[gl_InstanceCustomIndexEXT].indices[3*gl_PrimitiveID+2];

    // Vertex v0 = vertexBufferHeap[gl_InstanceCustomIndexEXT].vertices[idx0];
    // Vertex v1 = vertexBufferHeap[gl_InstanceCustomIndexEXT].vertices[idx1];
    // Vertex v2 = vertexBufferHeap[gl_InstanceCustomIndexEXT].vertices[idx2];

    // PrimitiveConstants primInfo = primitiveConstants[gl_InstanceCustomIndexEXT];

    // vec3 pos = v0.position*bc.x+v1.position*bc.y+v2.position*bc.z;

    // payload.p = vec4(gl_ObjectToWorldEXT * vec4(pos, 1.0), 1.0);
}