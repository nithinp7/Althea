
#version 450

layout(location = 0) in vec2 baseColorUV;
layout(location = 1) in vec2 normalMapUV;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D baseColorTexture;
layout(binding = 3) uniform sampler2D normalMapTexture;

void main() {
  outColor = texture(baseColorTexture, baseColorUV);
}