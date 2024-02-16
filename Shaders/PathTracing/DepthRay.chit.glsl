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

#include <GlobalIllumination/GIResources.glsl>
#include "BRDF.glsl"

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