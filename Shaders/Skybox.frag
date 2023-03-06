#version 450

#define PI 3.14159265359

layout(location=0) smooth in vec3 direction;

layout(location=0) out vec4 color;

layout(set=0, binding=0) uniform sampler2D environmentMap;

layout(set=0, binding=4) uniform UniformBufferObject {
  mat4 projection;
  mat4 inverseProjection;
  mat4 view;
  mat4 inverseView;
  vec3 lightDir;
  float time;
  float exposure;
} ubo;

vec3 sampleEnvMap(vec3 direction) {
  float yaw = atan(direction.z, direction.x);
  float pitch = -atan(direction.y, length(direction.xz));
  vec2 uv = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, uv, 0.0).rgb;
} 

void main() {
  color = vec4(sampleEnvMap(direction), 1.0);
  
  // color.rgb = color.rgb / (vec3(1.0) + color.rgb);
  
  color.rgb = vec3(1.0) - exp(-color.rgb * ubo.exposure);
}