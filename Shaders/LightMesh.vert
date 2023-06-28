#version 450

// Per-vertex attributes
layout(location=0) in vec3 vertPos;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <GlobalUniforms.glsl>

#define POINT_LIGHTS_SET 0
#define POINT_LIGHTS_BINDING 5
#include <PointLights.glsl>

layout(location=0) out vec3 color;

void main() {
  PointLight light = pointLightArr[gl_InstanceIndex];

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