#ifndef _GIRESOURCES_
#define _GIRESOURCES_

#define EMPTY_SLOT 0
#define RESERVED_SLOT 0xFFFFFFFF

#include <Misc/Hash.glsl>

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable

layout(set=0, binding=0) uniform sampler2D environmentMap; 
layout(set=0, binding=1) uniform sampler2D prefilteredMap; 
layout(set=0, binding=2) uniform sampler2D irradianceMap;
layout(set=0, binding=3) uniform sampler2D brdfLut;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <Global/GlobalUniforms.glsl>

// #define POINT_LIGHTS_SET 0
// #define POINT_LIGHTS_BINDING 5
// #include <PointLights.glsl>

// layout(set=0, binding=5) uniform samplerCubeArray shadowMapArray;

layout(set=0, binding=5) uniform sampler2D textureHeap[];

#define PRIMITIVE_CONSTANTS_SET 0
#define PRIMITIVE_CONSTANTS_BINDING 6
#include <PrimitiveConstants.glsl>

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

struct ProbeSlot {
  vec4 irradiance;
  int gridX;
  int gridY;
  int gridZ;
  int dbg;
};

struct Probe {
  ProbeSlot slots[4];
};

layout(set=1, binding=0) uniform accelerationStructureEXT acc;

// Output image
layout(set=1, binding=1) uniform sampler2D prevImg;
// Motion vectors
layout(set=1, binding=2, rgba32f) uniform image2D img;
// Prev depth buffer
layout(set=1, binding=3) uniform sampler2D prevDepthBuffer;
// Current depth buffer
layout(set=1, binding=4, r32f) uniform image2D depthBuffer;

layout(set=1, binding=5) uniform GI_UNIFORMS {
  uint spatialHashSize;
  uint spatialHashSlotsPerBuffer;
  uint probeCount;
  uint probesPerBuffer;

  uint gridCellSize;
  float padding1;
  float padding2;
  float padding3;
};

layout(std430, set=1, binding=6) buffer LIGHT_PROBES 
{
  Probe probes[];
} probeHeap[];

layout(std430, set=1, binding=7) buffer PROBE_HASH_GRID
{
  uint hashGrid[];
} hashHeap[];

struct FreeList {
  uint counter;
};
layout(std430, set=1, binding=8) buffer FREE_LIST
{
  FreeList freeList;
};

layout(set=1, binding=9, rgba32f) uniform image2D debugTarget;

layout(push_constant) uniform PathTracePushConstants {
  uint frameNumber; // frames since camera moved
} pushConstants;

#define baseColorTexture textureHeap[5*gl_InstanceCustomIndexEXT+0]
#define normalMapTexture textureHeap[5*gl_InstanceCustomIndexEXT+1]
#define metallicRoughnessTexture textureHeap[5*gl_InstanceCustomIndexEXT+2]
#define occlusionTexture textureHeap[5*gl_InstanceCustomIndexEXT+3]
#define emissiveTexture textureHeap[5*gl_InstanceCustomIndexEXT+4]

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