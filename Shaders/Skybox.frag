#version 450

#define PI 3.14159265359

layout(location=0) smooth in vec3 direction;

layout(location=0) out vec4 color;

layout(set=0, binding=0) uniform sampler2D environmentMap;

vec3 sampleEnvMap(vec3 direction) {
  float yaw = atan(direction.z, direction.x);
  float pitch = -atan(direction.y, length(direction.xz));
  vec2 uv = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return texture(environmentMap, uv).rgb;
} 

void main() {
  vec3 hdrSample = sampleEnvMap(direction);
  color = vec4(hdrSample / (hdrSample + vec3(1.0)), 1.0);

}