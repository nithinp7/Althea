
#version 450

layout(location=0) in vec3 worldPos;
layout(location=1) in vec3 color;

layout(location=0) out vec4 GBuffer_Position;
layout(location=1) out vec4 GBuffer_Normal;
layout(location=2) out vec4 GBuffer_Albedo;
layout(location=3) out vec4 GBuffer_MetallicRoughnessOcclusion;

void main() {
  GBuffer_Albedo = vec4(color, 1.0);
  GBuffer_Position = vec4(worldPos, 1.0);
  GBuffer_Normal = vec4(1.0, 0.0, 0.0, 1.0);
  // TODO: Should occlusion be 1 or 0 by default 
  GBuffer_MetallicRoughnessOcclusion = vec4(0.0, 1.0, 1.0, 1.0);
}
