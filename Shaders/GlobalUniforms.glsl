
#ifndef GLOBAL_UNIFORMS_SET
#define GLOBAL_UNIFORMS_SET 0
#endif

#ifndef GLOBAL_UNIFORMS_BINDING
#define GLOBAL_UNIFORMS_BINDING 0
#endif

#ifdef CUBEMAP_MULTIVIEW
#extension GL_EXT_multiview : enable
#endif

layout(set=GLOBAL_UNIFORMS_SET, binding=GLOBAL_UNIFORMS_BINDING) uniform UniformBufferObject {
  mat4 projection;
  mat4 inverseProjection;
#ifdef CUBEMAP_MULTIVIEW // TODO: Is this still being used? Maybe in point light shadows?
  mat4 views[6];
  mat4 inverseViews[6];
#else
  mat4 view;
  mat4 prevView;
  mat4 inverseView;
  mat4 prevInverseView;
#endif
  int lightCount;
  float time;
  float exposure;
} globals;