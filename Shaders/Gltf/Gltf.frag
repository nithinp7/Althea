
#version 460 core

#define PI 3.14159265359

layout(location=0) in vec2 uvs[4];
layout(location=4) in mat3 fragTBN;
layout(location=7) in vec3 worldPos;
layout(location=8) in vec3 direction;

layout(location=0) out vec4 GBuffer_Normal;
layout(location=1) out vec4 GBuffer_Albedo;
layout(location=2) out vec4 GBuffer_MetallicRoughnessOcclusion;

#include <Bindless/GlobalHeap.glsl>
#include <Global/GlobalUniforms.glsl>
#include <Global/GlobalResources.glsl>
#include <InstanceData/InstanceData.glsl>

layout(push_constant) uniform PushConstants {
  uint matrixBufferHandle;
  uint primConstantsBuffer;
  uint globalResourcesHandle;
  uint globalUniformsHandle;
} pushConstants;

#define globals RESOURCE(globalUniforms, pushConstants.globalUniformsHandle)
#define resources RESOURCE(globalResources, pushConstants.globalResourcesHandle)

void main() {
  Vertex v;
  v.position = vec3(0.0); // doesn't matter here
  v.tangent = fragTBN[0];
  v.bitangent = fragTBN[1];
  v.normal = fragTBN[2];
  v.uvs = uvs;

  MaterialDesc material = fetchMaterialFromPrimitive(v, pushConstants.primConstantsBuffer);

  if (material.bShouldDiscard) {
    discard;
  }

  float alpha = material.baseColor.a;

  GBuffer_Albedo = material.baseColor;
  GBuffer_Normal = vec4(material.normal, alpha);
  GBuffer_MetallicRoughnessOcclusion = 
      vec4(material.metallic, material.roughness, material.ao, alpha);
  // TODO: emissive...
}
