#pragma once

#include "Allocator.h"
#include "DescriptorSet.h"
#include "Library.h"
#include "StructuredBuffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace AltheaEngine {
class Application;
class Model;

class BufferHeap {
public:
  static BufferHeap CreateVertexBufferHeap(const std::vector<Model>& models);
  static BufferHeap CreateIndexBufferHeap(const std::vector<Model>& models);

  BufferHeap() = default;

  uint32_t getSize() const { return this->_size; }
  uint32_t getCapacity() const { return this->_capacity; }

  const std::vector<VkDescriptorBufferInfo>& getBufferInfos() {
    return this->_bufferInfos;
  }

protected:
  void _initBufferInfo(
      VkBuffer buffer,
      size_t offset,
      size_t size,
      uint32_t slotIdx);

  std::vector<VkDescriptorBufferInfo> _bufferInfos{};

  uint32_t _size;
  uint32_t _capacity;
};

template <typename TElement> class StructuredBufferHeap : public BufferHeap {
public:
  StructuredBufferHeap() = default;

  StructuredBufferHeap(std::vector<StructuredBuffer<TElement>>&& buffers)
      : _buffers(std::move(buffers)) {

    this->_size = this->_buffers.size();
    this->_capacity = this->_buffers.size();

    this->_bufferInfos.resize(this->_size);

    for (uint32_t bufferIdx = 0; bufferIdx < this->_size; ++bufferIdx) {
      this->_initBufferInfo(
          this->_buffers[bufferIdx].getAllocation().getBuffer(),
          0,
          this->_buffers[bufferIdx].getSize(),
          bufferIdx);
    }
  }

  const StructuredBuffer<TElement>& getBuffer(uint32_t bufferIdx) const {
    return this->_buffers[bufferIdx];
  }

  StructuredBuffer<TElement>& getBuffer(uint32_t bufferIdx) {
    return this->_buffers[bufferIdx];
  }

  const std::vector<StructuredBuffer<TElement>>& getBuffers() const {
    return this->_buffers;
  }

  void upload(const Application& app, VkCommandBuffer commandBuffer) {
    for (auto& buffer : this->_buffers)
      buffer.upload(app, commandBuffer);
  }

private:
  std::vector<StructuredBuffer<TElement>> _buffers;
};
} // namespace AltheaEngine