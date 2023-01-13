
#version 450

#define PI 3.14159265359

layout(location=0) in vec2 baseColorUV;
layout(location=1) in vec2 normalMapUV;
layout(location=2) out vec2 metallicRoughnessUV;
layout(location=3) out vec2 occlusionUV;
layout(location=4) out vec2 emissiveUV;
layout(location=5) in mat3 fragTBN;
layout(location=8) in vec3 direction;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform samplerCube skyboxTexture; 

layout(set=0, binding=1) uniform UniformBufferObject {
  mat4 projection;
  mat4 inverseProjection;
  mat4 view;
  mat4 inverseView;
  float time;
} globals;

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



float vary(float period, float rangeMin, float rangeMax) {
  float halfRange = 0.5 * (rangeMax - rangeMin);
  float t = 2.0 * radians(180.0) * globals.time / period;
  return halfRange * sin(t) + halfRange;
}

float vary(float period, float rangeMax) {
  return vary(period, 0.0, rangeMax);
}

float vary(float period) {
  return vary(period, 0.0, 1.0);
}

// TODO: lightdir should be removed as arguments here, should be a global
// uniform

// Specular BDRF
float specularBrdf(vec3 lightDir, vec3 normal, float roughness) {
  // half-vector
  vec3 H = normalize(lightDir - direction);
  
  float a2 = pow(roughness, 4);
  float NdotH = dot(normal, H);
  float D = a2 * max(0, NdotH) / pow(PI * NdotH * NdotH * (a2 - 1.0) + 1, 2);

  // TODO: break-down these terms further
  float NdotL = dot(normal, lightDir);
  float NdotV = dot(normal, -direction);
  float G_num = 2 * abs(NdotL) * max(0, dot(H, lightDir)) * 2 * abs(NdotV) * max(0, dot(H, -direction));
  float G_denom = (abs(dot(normal, lightDir)) + sqrt(a2 + (1 - a2) * pow(NdotL, 2))) * (abs(NdotV) + sqrt(a2 + (1 - a2) * pow(NdotV, 2)));

  float G = G_num / G_denom;
  float V = G / (4 * abs(NdotL) * abs(NdotV));

  return V * D;
}

vec3 diffuseBrdf(vec3 color) {
  return color / PI; 
} 

vec3 conductorFresnel(vec3 lightDir, vec3 f0, float bsdf) {
  vec3 H = normalize(lightDir - direction);
  float VdotH = dot(lightDir, H);

  return bsdf * (f0 + (1.0 - f0) * pow(1.0 - abs(VdotH), 5)); 
}

vec3 fresnelMix(vec3 lightDir, float ior, vec3 base, float layer) {
  vec3 H = normalize(lightDir - direction);
  float VdotH = dot(lightDir, H);

  float f0 = pow((1.0 - ior) / (1.0 + ior), 2);
  float fr = f0 + pow((1.0 - f0) * (1.0 - abs(VdotH)), 5);
  return mix(base, vec3(layer), fr);
}

void main() {
  // TODO: generalize, currently hardcoded in _model_ space
  // Direction towards (infinitely far-away) light
  vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));

  vec3 normalMapSample = texture(normalMapTexture, normalMapUV).rgb;
  vec3 tangentSpaceNormal = 
      (2.0 * normalMapSample - 1.0) *
      vec3(constants.normalScale, constants.normalScale, 1.0);
  vec3 normal = normalize(fragTBN * tangentSpaceNormal);

  vec3 reflectedDirection = reflect(normalize(direction), normal);
  vec4 reflectedColor = texture(skyboxTexture, reflectedDirection);
  // vec4 reflectedColor = textureLod(skyboxTexture, reflectedDirection, vary(14.0, 8.0));

  float ambientOcclusion = 
      texture(occlusionTexture, occlusionUV).r * constants.occlusionStrength;

  // float intensity = 2.0 * max(0, dot(lightDir, normal)) + 0.1;
  vec4 baseColor = texture(baseColorTexture, baseColorUV) * constants.baseColorFactor;



  vec2 metallicRoughness = 
      texture(metallicRoughnessTexture, metallicRoughnessUV).bg *
      vec2(constants.metallicFactor, constants.roughnessFactor);

  float metallic = metallicRoughness.x;
  float roughness = metallicRoughness.y;

  vec3 diffuse = diffuseBrdf(baseColor.rgb); 
  float specular = specularBrdf(lightDir, normal, roughness);

  
  vec3 metalBrdf = conductorFresnel(lightDir, diffuse, specular);
  vec3 dielectricBrdf = fresnelMix(lightDir, 1.5, diffuse, specular);

  vec3 material = mix(dielectricBrdf, metalBrdf, metallic);

  outColor = vec4(material, 1.0);
  // outColor = vec4(mix(baseColor, reflectedColor, vary(2.0)).rgb, 1.0);
  // outColor = vec4(metallicRoughness, 0.0, 1.0);
}
