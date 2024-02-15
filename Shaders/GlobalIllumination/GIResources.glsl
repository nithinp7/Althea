#ifndef _GIRESOURCES_
#define _GIRESOURCES_

#define EMPTY_SLOT 0
#define RESERVED_SLOT 0xFFFFFFFF

#include <Misc/Hash.glsl>

#include <Bindless/GlobalHeap.glsl>
#include <Global/GlobalResources.glsl>
#include <Global/GlobalUniforms.glsl>
#include <PrimitiveConstants.glsl>

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable

layout(push_constant) uniform PushConstant {
  uint globalResourcesHandle;
  uint globalUniformsHandle;
  uint tlasHandle;

  uint prevImgHandle;
  uint imgHandle;
  uint prevDepthBufferHandle;
  uint depthBufferHandle;
  
  uint framesSinceCameraMoved;
} pushConstants;

SAMPLER2D(textureHeap);

#define resources RESOURCE(globalResources, pushConstants.globalResourcesHandle)
#define globals RESOURCE(globalUniforms, pushConstants.globalUniformsHandle)

#define environmentMap textureHeap[resources.ibl.environmentMapHandle]
#define prefilteredMap textureHeap[resources.ibl.prefilteredMapHandle]
#define irradianceMap textureHeap[resources.ibl.irradianceMapHandle]
#define brdfLut textureHeap[resources.ibl.brdfLutHandle]

#define getPrimitive(primIdx) \
    RESOURCE(primitiveConstants, resources.primitiveConstantsBuffer) \
      .primitiveConstantsArr[primIdx]

#define baseColorTexture(primIdx) \
    textureHeap[getPrimitive(primIdx).baseTextureHandle]
#define normalMapTexture(primIdx) \
    textureHeap[getPrimitive(primIdx).normalTextureHandle]
#define metallicRoughnessTexture(primIdx) \
    textureHeap[getPrimitive(primIdx).metallicRoughnessTextureHandle]
#define occlusionTexture(primIdx) \
    textureHeap[getPrimitive(primIdx).occlusionTextureHandle]
#define emissiveTexture(primIdx) \
    textureHeap[getPrimitive(primIdx).emissiveTextureHandle]

IMAGE2D_W(imageHeap);

#define prevImg textureHeap[pushConstants.prevImgHandle]
#define img imageHeap[pushConstants.imgHandle]
#define prevDepthBuffer textureHeap[pushConstants.prevDepthBufferHandle]
#define depthBuffer imageHeap[pushConstants.depthBufferHandle]

TLAS(tlasHeap);

#define acc tlasHeap[pushConstants.tlasHandle]

struct Vertex {
  vec3 position;
  vec3 tangent;
  vec3 bitangent;
  vec3 normal;
  vec2 uvs[4];
};

#extension GL_EXT_scalar_block_layout : enable
layout(scalar, set=0, binding=7) readonly buffer VERTEX_BUFFER_HEAP { Vertex vertices[]; } vertexBufferHeap[];
layout(set=0, binding=8) readonly buffer INDEX_BUFFER_HEAP { uint indices[]; } indexBufferHeap[];

// struct ProbeSlot {
//   vec4 irradiance;
//   int gridX;
//   int gridY;
//   int gridZ;
//   int dbg;
// };

// struct Probe {
//   ProbeSlot slots[4];
// };

// layout(set=1, binding=5) uniform GI_UNIFORMS {
//   uint spatialHashSize;
//   uint spatialHashSlotsPerBuffer;
//   uint probeCount;
//   uint probesPerBuffer;

//   uint gridCellSize;
//   float padding1;
//   float padding2;
//   float padding3;
// };

#define INTERPOLATE(member)(v.member=v0.member*bc.x+v1.member*bc.y+v2.member*bc.z)

// void writeSpatialHash(vec3 pos, vec3 color) {
//   ivec3 gridPosi = ivec3(floor(pos / gridCellSize));

//   uint hash = hashCoords(gridPosi.x, gridPosi.y, gridPosi.z);
//   uint hashSlotIdx = hash % spatialHashSize;
  
//   uint hashBufferIdx = hashSlotIdx / spatialHashSlotsPerBuffer;
//   uint hashSlotLocalIdx = hashSlotIdx % spatialHashSlotsPerBuffer;
//   uint prevVal = atomicCompSwap(hashHeap[hashBufferIdx].hashGrid[hashSlotLocalIdx], EMPTY_SLOT, RESERVED_SLOT);
//   // if (prevVal == RESERVED_SLOT)
//   // {
//   //   // Already being allocated from another thread
//   //   return;
//   // }
//   // if (prevVal != EMPTY_SLOT)
//   // {
//   //   // Bucket allocation already exists
//   //   // TODO: Append color info in existing buckets
//   //   return;
//   // }
//   // else
//   {
//     // uint prevCounter = atomicAdd(freeLists[gl_SubgroupInvocationID],1);
//     // uint bucketIdx = prevCounter + gl_SubgroupInvocationID;
//     uint bucketIdx = atomicAdd(freeList.counter, 1) % probeCount;

//     hashHeap[hashBufferIdx].hashGrid[hashSlotLocalIdx] = bucketIdx;

//     uint bucketBufferIdx = bucketIdx / probesPerBuffer;
//     uint bucketLocalIdx = bucketIdx % probesPerBuffer;

//     ProbeBucket bucket;
//     bucket.probes[0].color = vec4(color, 1.0);
    
//     probeHeap[bucketBufferIdx].buckets[bucketLocalIdx] = bucket;
//   }
// }

// vec3 readSpatialHash(vec3 pos) {
//   ivec3 gridPosi = ivec3(floor(pos / gridCellSize));

//   uint hash = hashCoords(gridPosi.x, gridPosi.y, gridPosi.z);
//   uint hashSlotIdx = hash % spatialHashSize;
  
//   uint hashBufferIdx = hashSlotIdx / spatialHashSlotsPerBuffer;
//   uint hashSlotLocalIdx = hashSlotIdx % spatialHashSlotsPerBuffer;
//   uint bucketIdx = hashHeap[hashBufferIdx].hashGrid[hashSlotLocalIdx];
 
//   if (bucketIdx != EMPTY_SLOT && bucketIdx != RESERVED_SLOT)
//   {
//     uint bucketBufferIdx = bucketIdx / probesPerBuffer;
//     uint bucketLocalIdx = bucketIdx % probesPerBuffer;

//     ProbeBucket bucket = probeHeap[bucketBufferIdx].buckets[bucketLocalIdx];
//     return bucket.probes[0].color.xyz;
//   }

//   return vec3(0.0);
// }

#endif // _GIRESOURCES_