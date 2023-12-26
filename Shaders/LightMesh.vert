#version 450

// Per-vertex attributes
layout(location=0) in vec3 vertPos;

#include <PointLights.glsl>

layout(location=0) out vec3 color;

layout(push_constant) uniform {
  uint uniformsHandle;
} pushConstants;

void main() {
  PointLightUniforms globals = RESOURCE(pointLightUniforms, pushConstants.uniformsHandle);
  PointLight light = RESOURCE(pointLights, globals.pointLightsBufferHandle).pointLightArr[gl_InstanceIndex];

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