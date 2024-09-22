// Closest hit shader
#version 460 core

#extension GL_EXT_ray_tracing : enable

#define IS_RT_SHADER
#include <SimpleRT/SimpleRTResources.glsl>

layout(location = 0) rayPayloadInEXT RtPayload payload;

void main() {
  payload.color = vec4(0.0, 0.0, 0.0, 1.0);
}