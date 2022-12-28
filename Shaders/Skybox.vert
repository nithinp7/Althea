#version 450

layout(location=0) smooth out vec3 direction;

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

void main() {
  vec2 screenPos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  vec4 pos = vec4(screenPos * 2.0f - 1.0f, 0.0f, 1.0f);

  mat4 invProj = inverse(ubo.proj);
  mat4 invView = inverse(ubo.view);
  direction = mat3(invView) * (invProj * pos).xyz;

  gl_Position = pos;
}