
#version 450

layout(location=0) in vec2 baseColorUV;
layout(location=1) in vec2 normalMapUV;
layout(location=2) in vec4 debugColor;
layout(location=3) in mat3 fragTBN;
layout(location=6) in vec3 direction;

layout(location=0) out vec4 outColor;

layout(set=0, binding=1) uniform samplerCube skyboxTexture; 

layout(set=1, binding=2) uniform sampler2D baseColorTexture;
layout(set=1, binding=3) uniform sampler2D normalMapTexture;

void main() {
  vec3 lightDir = normalize(vec3(1.0, -1.0, 1.0));

  vec3 tangentSpaceNormal = 2.0 * texture(normalMapTexture, normalMapUV).rgb - 1.0;
  vec3 normal = fragTBN * tangentSpaceNormal;

  float intensity = 2.0 * max(0, dot(lightDir, normal)) + 0.1;

  vec4 reflectedColor = texture(skyboxTexture, direction);

  vec4 baseColor = vec4(intensity * texture(baseColorTexture, baseColorUV).rgb, 1.0);

  outColor = 0.5 * reflectedColor + 0.5 * baseColor;
}