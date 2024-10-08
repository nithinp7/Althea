#version 450

layout(location=0) smooth out vec3 direction;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include "GlobalUniforms.glsl"

void main() {
  vec2 screenPos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  vec4 pos = vec4(screenPos * 2.0f - 1.0f, 0.0f, 1.0f);

#ifdef CUBEMAP_MULTIVIEW
  direction = 
      mat3(globals.inverseViews[gl_ViewIndex]) * (globals.inverseProjection * pos).xyz;
#else
  direction = mat3(globals.inverseView) * (globals.inverseProjection * pos).xyz;
#endif

  gl_Position = pos;
}