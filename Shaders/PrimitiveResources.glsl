#ifndef _PRIMITIVERESOURCES_
#define _PRIMITIVERESOURCES_

#define IS_SHADER
#include <../Include/Althea/Common/GltfCommon.h>

SAMPLER2D(primTexHeap);

BUFFER_R(primitiveConstants, PrimitiveConstantsResource{
  PrimitiveConstants primitiveConstantsArr[];
});

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

BUFFER_R(matrixHeap, MatrixBuffer{
  mat4 matrices[];
});
#define getMatrix(bufferIdx, matrixIdx) \
    matrixHeap[bufferIdx].matrices[matrixIdx]

BUFFER_R(jointMapHeap, JointMapBuffer{
  uint jointToNode[];
});
#define getNodeIdxFromJointIdx(jointMapHandle, jointIdx) \
    jointMapHeap[jointMapHandle].jointToNode[jointIdx];

#endif // _PRIMITIVERESOURCES_