#version 450

layout(location=0) smooth out vec3 direction;

layout(set=0, binding=2) uniform UniformBufferObject {
  mat4 projection;
  mat4 inverseProjection;
  mat4 view;
  mat4 inverseView;
} ubo;

void main() {
  vec2 screenPos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  vec4 pos = vec4(screenPos * 2.0f - 1.0f, 0.0f, 1.0f);

  direction = mat3(ubo.inverseView) * (ubo.inverseProjection * pos).xyz;

  gl_Position = pos;
}