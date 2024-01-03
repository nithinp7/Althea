#version 460 core

layout(location=0) smooth out vec3 direction;
layout(location=1) out vec2 screenUV;

#include <Global/GlobalUniforms.glsl>

layout(push_constant) uniform PushConstants {
  uint globalUniformsHandle;
  uint globalResourcesHandle;
} pushConstants;

#define globals RESOURCE(globalUniforms, pushConstants.globalUniformsHandle)

void main() {
  vec2 screenPos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  screenUV = screenPos;
  vec4 pos = vec4(screenPos * 2.0f - 1.0f, 0.0f, 1.0f);

#ifdef CUBEMAP_MULTIVIEW
  direction = 
      mat3(globals.inverseViews[gl_ViewIndex]) * (globals.inverseProjection * pos).xyz;
#else
  direction = mat3(globals.inverseView) * (globals.inverseProjection * pos).xyz;
#endif

  gl_Position = pos;
}