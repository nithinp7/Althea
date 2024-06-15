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

  StructuredBufferHeap(
      const Application& app,
      uint32_t bufferCount,
      uint32_t elemsPerBuffer) {
    this->_size = bufferCount;
    this->_capacity = bufferCount;

    this->_bufferInfos.resize(bufferCount);
    this->_buffers.reserve(bufferCount);

    for (uint32_t i = 0; i < bufferCount; ++i) {
      _buffers.emplace_back(app, elemsPerBuffer);
      _initBufferInfo(
          this->_buffers[i].getAllocation().getBuffer(),
          0,
          this->_buffers[i].getSize(),
          i);
    }
  }

  void zeroBuffer(VkCommandBuffer commandBuffer) {
    for (auto& buffer : _buffers)
      buffer.zeroBuffer(commandBuffer);
  }

  void registerToHeap(GlobalHeap& heap) {
    for (auto& buffer : _buffers)
      buffer.registerToHeap(heap);
  }

  BufferHandle getFirstBufferHandle() const { return _buffers[0].getHandle(); }

  void rwBarrier(VkCommandBuffer commandBuffer) const {
    for (const auto& buf : _buffers)
      buf.rwBarrier(commandBuffer);
  }

  void barrier(
      VkCommandBuffer commandBuffer,
      VkAccessFlags srcAccessFlags,
      VkPipelineStageFlags srcPipelineStageFlags,
      VkAccessFlags dstAccessFlags,
      VkPipelineStageFlags dstPipelineStageFlags) {
    for (const auto& buf : _buffers)
      buf.barrier(
          commandBuffer,
          srcAccessFlags,
          srcPipelineStageFlags,
          dstAccessFlags,
          dstPipelineStageFlags);
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

  void setElement(const TElement& element, uint32_t i) {
    size_t elemsPerBuffer = _buffers[0].getCount();
    _buffers[i / elemsPerBuffer].setElement(element, i % elemsPerBuffer);
  }

  size_t getBufferCount() const { return _buffers.size(); }

  void upload(Application& app, VkCommandBuffer commandBuffer) {
    for (auto& buffer : this->_buffers)
      buffer.upload(app, commandBuffer);
  }

private:
  std::vector<StructuredBuffer<TElement>> _buffers;
};
} // namespace AltheaEngine