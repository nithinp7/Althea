#pragma once

#include "DescriptorSet.h"
#include "Allocator.h"
#include "Library.h"

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

private:
  void _initBufferInfo(VkBuffer buffer, size_t offset, size_t size, uint32_t slotIdx);

  std::vector<VkDescriptorBufferInfo> _bufferInfos{};

  uint32_t _size;
  uint32_t _capacity;
};
} // namespace AltheaEngine