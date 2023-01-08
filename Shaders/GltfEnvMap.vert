
#version 450

layout(location=0) in vec3 position;
layout(location=1) in mat3 tbn;
// layout(location = 1) in vec3 tangent;
// layout(location = 2) in vec3 bitangent;
// layout(location = 3) in vec3 normal;
layout(location=4) in vec2 uvs[4];

layout(location=0) out vec2 baseColorUV;
layout(location=1) out vec2 normalMapUV;
layout(location=2) out float normalScale;
layout(location=3) out mat3 vertTbn;
layout(location=6) out vec3 direction;

layout(set=1, binding=0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

layout(set=1, binding=1) uniform ConstantBufferObject {
  int baseTextureCoordinateIndex;
  int normalMapTextureCoordinateIndex;
  float normalScale;
} constants;

void main() {
  // TODO: Do this on CPU?
  mat4 invView = inverse(ubo.view);
  vec3 cameraPos = invView[3].xyz;

  vec4 worldPos = ubo.model * vec4(position, 1.0);

  direction = worldPos.xyz - cameraPos;

  gl_Position = ubo.proj * ubo.view * worldPos;
  baseColorUV = uvs[constants.baseTextureCoordinateIndex];
  normalMapUV = uvs[constants.normalMapTextureCoordinateIndex];
  normalScale = constants.normalScale;
  vertTbn = mat3(ubo.model) * tbn;
}