// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Closest hit shader
#version 460 core

#extension GL_EXT_ray_tracing : enable
// TODO: enable this ext in vulkan
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec4 payload;
hitAttributeEXT vec2 attribs;

#ifndef TEXTURE_HEAP_COUNT
#define TEXTURE_HEAP_COUNT 1
#endif

layout(set=0, binding=7) uniform sampler2D textureHeap[TEXTURE_HEAP_COUNT];

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
layout(scalar, set=0, binding=9) readonly buffer VERTEX_BUFFER_HEAP { Vertex vertices[]; } vertexBufferHeap[VERTEX_BUFFER_HEAP_COUNT];
layout(set=0, binding=10) readonly buffer INDEX_BUFFER_HEAP { uint indices[]; } indexBufferHeap[INDEX_BUFFER_HEAP_COUNT];

void main() {
    vec3 bc = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    uint idx0 = indexBufferHeap[gl_InstanceCustomIndexEXT].indices[3*gl_PrimitiveID+0];
    uint idx1 = indexBufferHeap[gl_InstanceCustomIndexEXT].indices[3*gl_PrimitiveID+1];
    uint idx2 = indexBufferHeap[gl_InstanceCustomIndexEXT].indices[3*gl_PrimitiveID+2];

    Vertex v0 = vertexBufferHeap[gl_InstanceCustomIndexEXT].vertices[idx0];
    Vertex v1 = vertexBufferHeap[gl_InstanceCustomIndexEXT].vertices[idx1];
    Vertex v2 = vertexBufferHeap[gl_InstanceCustomIndexEXT].vertices[idx2];

    // TODO: Implement interpolation for all elems of vertex
     
    PrimitiveConstants primInfo = primitiveConstants[gl_InstanceCustomIndexEXT];
    // TODO: generalize
    int i = 0;//\primInfo.baseTextureCoordinateIndex;
    vec2 uv = v0.uvs[i] * bc.x + v1.uvs[i] * bc.y + v2.uvs[i] * bc.z;

    // vec3 color = vec3(float(gl_PrimitiveID % 1000) / 1000.0);//texture(textureHeap[5*gl_InstanceCustomIndexEXT], uv).rgb;
    vec3 color = texture(textureHeap[5*gl_InstanceCustomIndexEXT], uv).rgb;

    payload = vec4(color, 1.0);
}