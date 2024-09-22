
#ifndef _INSTANCEDATA_
#define _INSTANCEDATA_

#define IS_SHADER
#include <Bindless/GlobalHeap.glsl>
#include <../Include/Althea/Common/InstanceDataCommon.h>

struct MaterialDesc {
  vec4 baseColor;
  float metallic;
  float roughness;
  float ao;
  float emissive;
  vec3 normal;
};

SAMPLER2D(_materialSamplerHeap);

DECL_CONSTANTS(MaterialConstants, materialConstants);
DECL_CONSTANTS(PrimitiveConstants, primitiveConstants);
DECL_BUFFER(R_PACKED, Vertex, primitiveVertices);
DECL_BUFFER(R_PACKED, uint, primitiveIndices);

MaterialDesc fetchMaterial(Vertex v, uint materialHandle) {
  MaterialDesc desc;

  BINDLESS(materialConstants, materialHandle);
  MaterialConstants constants = GET_CONSTANTS(materialConstants);

  desc.baseColor = texture(
      _materialSamplerHeap[constants.baseTextureHandle], 
      v.uvs[constants.baseTextureCoordinateIndex]) * constants.baseColorFactor;

  vec3 normalMapSample = texture(
      _materialSamplerHeap[constants.normalTextureHandle], 
      v.uvs[constants.baseTextureCoordinateIndex]).rgb;
  vec3 tangentSpaceNormal = 
      (2.0 * normalMapSample - 1.0) *
      vec3(constants.normalScale, constants.normalScale, 1.0);
  
  desc.normal = 
      normalize(gl_ObjectToWorldEXT *
      vec4(normalize(
        tangentSpaceNormal.x * v.tangent + 
        tangentSpaceNormal.y * v.bitangent + 
        tangentSpaceNormal.z * v.normal), 0.0));

  vec2 metallicRoughness = texture(
      _materialSamplerHeap[constants.metallicRoughnessTextureHandle], 
      v.uvs[constants.metallicRoughnessTextureCoordinateIndex]).bg *
        vec2(constants.metallicFactor, constants.roughnessFactor);

  desc.metallic = metallicRoughness.x;
  desc.roughness = metallicRoughness.y;

  // defaults for now
  desc.ao = 0.0;
  desc.emissive = 0.0;

  return desc;
}

PrimitiveConstants getPrimitiveConstants(uint primitiveHandle) {
  BINDLESS(primitiveConstants, primitiveHandle);
  return GET_CONSTANTS(primitiveConstants);
}

MaterialDesc fetchMaterialFromPrimitive(Vertex v, uint primitiveHandle) {
  BINDLESS(primitiveConstants, primitiveHandle);
  PrimitiveConstants constants = GET_CONSTANTS(primitiveConstants);

  return fetchMaterial(v, constants.materialHandle);
}

#define INTERPOLATE(member)(v.member=v0.member*bc.x+v1.member*bc.y+v2.member*bc.z)

Vertex fetchInterpolatedVertex(uint triangleIdx, vec3 bc, uint primitiveHandle) {
  BINDLESS(primitiveConstants, primitiveHandle);
  PrimitiveConstants constants = GET_CONSTANTS(primitiveConstants);

  BINDLESS(primitiveVertices, constants.vertexBufferHandle);
  BINDLESS(primitiveIndices, constants.indexBufferHandle);
    
  uint idx0 = BUFFER_GET(primitiveIndices, 3*triangleIdx+0);
  uint idx1 = BUFFER_GET(primitiveIndices, 3*triangleIdx+1);
  uint idx2 = BUFFER_GET(primitiveIndices, 3*triangleIdx+2);

  Vertex v0 = BUFFER_GET(primitiveVertices, idx0);
  Vertex v1 = BUFFER_GET(primitiveVertices, idx1);
  Vertex v2 = BUFFER_GET(primitiveVertices, idx2);

  Vertex v;
  INTERPOLATE(position);
  INTERPOLATE(tangent);
  INTERPOLATE(bitangent);
  INTERPOLATE(normal);
  INTERPOLATE(uvs[0]);
  INTERPOLATE(uvs[1]);
  INTERPOLATE(uvs[2]);
  INTERPOLATE(uvs[3]);

  return v;
}
#endif // _INSTANCEDATA_