#version 460 core

// Per-vertex attributes
layout(location=0) in vec3 vertPos;

#include <Global/GlobalUniforms.glsl>
#include <PointLights.glsl>

layout(location=0) out vec3 color;

layout(push_constant) uniform PushConstants {
  uint globalUniformsHandle;
  uint lightBufferHandle;
} pushConstants;

#define globals RESOURCE(globalUniforms, pushConstants.globalUniformsHandle)

void main() {
  PointLight light = 
      RESOURCE(pointLights, pushConstants
      .lightBufferHandle).pointLightArr[gl_InstanceIndex];

  vec3 cameraPos = globals.inverseView[3].xyz;
  float radius = 1.0;
  vec3 worldPos = light.position + radius * vertPos; 
  
  gl_Position = globals.projection * globals.view * vec4(worldPos, 1.0);

#ifndef SKIP_TONEMAP
  color = vec3(1.0) - exp(-light.emission * globals.exposure);
#elif
  color = light.emission;
#endif
}