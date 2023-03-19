#version 450

#define PI 3.14159265359

layout(location=0) smooth in vec3 direction;

layout(location=0) out vec4 color;

layout(set=0, binding=0) uniform sampler2D environmentMap;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include "GlobalUniforms.glsl"

vec3 sampleEnvMap(vec3 direction) {
  float yaw = atan(direction.z, direction.x);
  float pitch = -atan(direction.y, length(direction.xz));
  vec2 uv = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, uv, 0.0).rgb;
} 

void main() {
  color = vec4(sampleEnvMap(direction), 1.0);
  
  // color.rgb = color.rgb / (vec3(1.0) + color.rgb);
  
#ifndef SKIP_TONEMAP
  color.rgb = vec3(1.0) - exp(-color.rgb * globals.exposure);
#endif
}