
#version 450

layout(location=0) in vec2 baseColorUV;
layout(location=1) in vec2 normalMapUV;
layout(location=2) out vec2 metallicRoughnessUV;
layout(location=3) out vec2 occlusionUV;
layout(location=4) out vec2 emissiveUV;
layout(location=5) in mat3 fragTBN;
layout(location=8) in vec3 direction;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform samplerCube skyboxTexture; 

// TODO: may be too big for inline block
layout(set=1, binding=0) uniform ConstantBufferObject {
  vec4 baseColorFactor;
  vec3 emissiveFactor;

  int baseTextureCoordinateIndex;
  int normalMapTextureCoordinateIndex;
  int metallicRoughnessTextureCoordinateIndex;
  int occlusionTextureCoordinateIndex;
  int emissiveTextureCoordinateIndex;

  float normalScale;
  float metallicFactor;
  float roughnessFactor;
  float occlusionStrength;

  float alphaCutoff;
} constants;

layout(set=1, binding=1) uniform sampler2D baseColorTexture;
layout(set=1, binding=2) uniform sampler2D normalMapTexture;
layout(set=1, binding=3) uniform sampler2D metallicRoughnessTexture;
layout(set=1, binding=4) uniform sampler2D occlusionTexture;
layout(set=1, binding=5) uniform sampler2D emissiveTexture;

void main() {
  vec3 lightDir = normalize(vec3(1.0, -1.0, 1.0));
  
  vec3 normalMapSample = texture(normalMapTexture, normalMapUV).rgb;
  vec3 tangentSpaceNormal = 
      (2.0 * normalMapSample - 1.0) *
      vec3(constants.normalScale, constants.normalScale, 1.0);
  vec3 normal = normalize(fragTBN * tangentSpaceNormal);

  vec3 reflectedDirection = reflect(normalize(direction), normal);
  vec4 reflectedColor = texture(skyboxTexture, reflectedDirection);

  vec2 metallicRoughness = 
      texture(metallicRoughnessTexture, metallicRoughnessUV).bg *
      vec2(constants.metallicFactor, constants.roughnessFactor);

  // float intensity = 2.0 * max(0, dot(lightDir, normal)) + 0.1;
  vec4 baseColor = texture(baseColorTexture, baseColorUV) * constants.baseColorFactor;

  outColor = vec4(mix(baseColor, reflectedColor, metallicRoughness.x).rgb, 1.0);
}