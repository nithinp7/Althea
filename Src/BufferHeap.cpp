#include "BufferHeap.h"

#include "Application.h"
#include "IndexBuffer.h"
#include "Model.h"
#include "Primitive.h"
#include "Texture.h"
#include "VertexBuffer.h"

// TODO: Implement partial updating of the heap

namespace AltheaEngine {

BufferHeap
BufferHeap::CreateVertexBufferHeap(const std::vector<Model>& models) {
  BufferHeap heap;

  heap._size = 0;
  heap._capacity = 0;

  for (const Model& model : models)
    heap._capacity += model.getPrimitivesCount();

  heap._size = heap._capacity;

  heap._bufferInfos.resize(heap._size);

  uint32_t bufferSlotIdx = 0;
  for (const Model& model : models) {
    for (const Primitive& prim : model.getPrimitives()) {
      const VertexBuffer<Vertex>& buffer = prim.getVertexBuffer();
      heap._initBufferInfo(
          buffer.getAllocation().getBuffer(),
          0,
          buffer.getVertexCount() * sizeof(Vertex),
          bufferSlotIdx++);
    }
  }

  return heap;
}

BufferHeap BufferHeap::CreateIndexBufferHeap(const std::vector<Model>& models) {
  BufferHeap heap;

  heap._size = 0;
  heap._capacity = 0;

  for (const Model& model : models)
    heap._capacity += model.getPrimitivesCount();

  heap._size = heap._capacity;

  heap._bufferInfos.resize(heap._size);

  uint32_t bufferSlotIdx = 0;
  for (const Model& model : models) {
    for (const Primitive& prim : model.getPrimitives()) {
      const IndexBuffer& buffer = prim.getIndexBuffer();
      heap._initBufferInfo(
          buffer.getAllocation().getBuffer(),
          0,
          buffer.getIndexCount() * sizeof(uint32_t),
          bufferSlotIdx++);
    }
  }

  return heap;
}

void BufferHeap::_initBufferInfo(
    VkBuffer buffer,
    size_t offset,
    size_t size,
    uint32_t slotIdx) {
  VkDescriptorBufferInfo& bufferInfo = this->_bufferInfos[slotIdx];

  bufferInfo.buffer = buffer;
  bufferInfo.offset = offset;
  bufferInfo.range = size;
}
} // namespace AltheaEngine