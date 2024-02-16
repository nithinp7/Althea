#ifndef _PRIMITIVECONSTANTS_
#define _PRIMITIVECONSTANTS_

struct PrimitiveConstants {
  vec4 baseColorFactor;
  vec4 emissiveFactor;

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

  uint baseTextureHandle;
  uint normalTextureHandle;
  uint metallicRoughnessTextureHandle;
  uint occlusionTextureHandle;
  uint emissiveTextureHandle;

  uint vertexBufferHandle;
  uint indexBufferHandle;

  uint padding1;
  uint padding2;
  uint padding3;
};


SAMPLER2D(primTexHeap);

#define getPrimitive(primIdx) \
    RESOURCE(primitiveConstants, resources.primitiveConstantsBuffer) \
      .primitiveConstantsArr[primIdx]

#define baseColorTexture(primIdx) \
    primTexHeap[getPrimitive(primIdx).baseTextureHandle]
#define normalMapTexture(primIdx) \
    primTexHeap[getPrimitive(primIdx).normalTextureHandle]
#define metallicRoughnessTexture(primIdx) \
    primTexHeap[getPrimitive(primIdx).metallicRoughnessTextureHandle]
#define occlusionTexture(primIdx) \
    primTexHeap[getPrimitive(primIdx).occlusionTextureHandle]
#define emissiveTexture(primIdx) \
    primTexHeap[getPrimitive(primIdx).emissiveTextureHandle]

BUFFER_R(primitiveConstants, PrimitiveConstantsResource{
  PrimitiveConstants primitiveConstantsArr[];
});

struct Vertex {
  vec3 position;
  vec3 tangent;
  vec3 bitangent;
  vec3 normal;
  vec2 uvs[4];
};

// Raw bindless heap stuff so we can use a scalar layout...
#extension GL_EXT_scalar_block_layout : enable
layout(scalar, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) readonly buffer VBResource{
  Vertex vertices[];
} vertexBufferHeap[];

#define getVertex(primIdx, vertexIdx) \
    vertexBufferHeap[getPrimitive(primIdx).vertexBufferHandle].vertices[vertexIdx]

BUFFER_R(indexBufferHeap, IBResource{
  uint indices[];
});

#define getIndex(primIdx, idx) \
    indexBufferHeap[getPrimitive(primIdx).indexBufferHandle].indices[idx]

#endif // _PRIMITIVECONSTANTS_