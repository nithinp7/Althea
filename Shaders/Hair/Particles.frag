
#version 450

layout(location=0) in vec3 normal;
layout(location=1) in vec3 color;

layout(location=0) out vec4 GBuffer_Normal;
layout(location=1) out vec4 GBuffer_Albedo;
layout(location=2) out vec4 GBuffer_MetallicRoughnessDebug;

void main() {
  GBuffer_Normal = vec4(normalize(normal), 1.0);
  GBuffer_Albedo = vec4(color, 1.0);
  GBuffer_MetallicRoughnessDebug = vec4(0.0, 0.05, 0.0, 1.0);
}
