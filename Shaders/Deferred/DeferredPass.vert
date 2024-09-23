#version 450

layout(location=0) smooth out vec3 direction;
layout(location=1) out vec2 uv;

layout(push_constant) uniform Push {
  uint globalResourcesHandle;
  uint globalUniformsHandle;
} push;

#include <Global/GlobalResources.glsl>
#include <Global/GlobalUniforms.glsl>
#define globals globalUniforms[push.globalUniformsHandle]

void main() {
  vec2 screenPos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  uv = screenPos;
  vec4 pos = vec4(screenPos * 2.0f - 1.0f, 0.0f, 1.0f);

  direction = mat3(globals.inverseView) * (globals.inverseProjection * pos).xyz;

  gl_Position = pos;
}