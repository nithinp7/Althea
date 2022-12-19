
#version 450

layout(location = 0) in vec2 baseColorUV;
layout(location = 1) in vec2 normalMapUV;
layout(location = 2) in vec4 debugColor;
layout(location = 3) in mat3 fragTBN;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D baseColorTexture;
layout(binding = 3) uniform sampler2D normalMapTexture;

void main() {
  vec3 lightDir = -vec3(1.0, -1.0, 5.0);

  vec3 tangentSpaceNormal = 2.0 * texture(normalMapTexture, normalMapUV).rgb - 1.0;
  vec3 normal = fragTBN * tangentSpaceNormal;

  float intensity = max(0, dot(lightDir, normal));

  outColor = vec4(intensity * texture(baseColorTexture, baseColorUV).rgb, 1.0);
}