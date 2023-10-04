// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Miss shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

#include "RayPayload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.colorOut = vec4(1.0, 0.0, 0.0, 1.0);
}