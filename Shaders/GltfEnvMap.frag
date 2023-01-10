
#version 450

layout(location=0) in vec2 baseColorUV;
layout(location=1) in vec2 normalMapUV;
layout(location=2) in float normalScale;
layout(location=3) in mat3 fragTBN;
layout(location=6) in vec3 direction;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform samplerCube skyboxTexture; 

layout(set=1, binding=1) uniform sampler2D baseColorTexture;
layout(set=1, binding=2) uniform sampler2D normalMapTexture;

void main() {
  vec3 lightDir = normalize(vec3(1.0, -1.0, 1.0));
  
  vec3 normalMapSample = texture(normalMapTexture, normalMapUV).rgb;
  vec3 tangentSpaceNormal = 
      (2.0 * normalMapSample - 1.0) *
      vec3(normalScale, normalScale, 1.0);
  vec3 normal = normalize(fragTBN * tangentSpaceNormal);

  float intensity = 2.0 * max(0, dot(lightDir, normal)) + 0.1;

  vec3 reflectedDirection = reflect(normalize(direction), normal);
  vec4 reflectedColor = texture(skyboxTexture, reflectedDirection);

  vec4 baseColor = vec4(intensity * texture(baseColorTexture, baseColorUV).rgb, 1.0);

  outColor = reflectedColor;
}