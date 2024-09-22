#version 450

#include "SimpleRTResources.glsl"

layout(location=0) smooth out vec3 direction;
layout(location=1) out vec2 screenUV;

void main() {
  vec2 screenPos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  screenUV = screenPos;
  vec4 pos = vec4(screenPos * 2.0f - 1.0f, 0.0f, 1.0f);

  direction = mat3(globals.inverseView) * (globals.inverseProjection * pos).xyz;

  gl_Position = pos;
}
