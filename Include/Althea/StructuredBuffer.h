#pragma once

#include "Allocator.h"
#include "BufferUtilities.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;

// TODO: Make CPU-copy of uniform buffer optional?

template <typename TElement> class ALTHEA_API StructuredBuffer {
public:
  StructuredBuffer() = default;

  StructuredBuffer(
      const Application& app,
      uint32_t count,
      VkBufferUsageFlags additionalFlags = 0) {
    this->_structureArray.resize(count);

    // TODO: This assumes that the uniform buffer will be _often_ rewritten
    // and perhaps in a random pattern. We should prefer a different type of
    // memory if the uniform buffer will mostly be persistent.
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    this->_allocation = BufferUtilities::createBuffer(
        app,
        sizeof(TElement) * count,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | additionalFlags,
        allocInfo);
  }

  void setElement(const TElement& element, uint32_t i) {
    this->_structureArray[i] = element;
  }

  void upload(const Application& app, VkCommandBuffer commandBuffer) const {
    size_t size = this->_structureArray.size() * sizeof(TElement);
    BufferAllocation staging = BufferUtilities::createStagingBuffer(
        app,
        gsl::span<const std::byte>(
            reinterpret_cast<const std::byte*>(this->_structureArray.data()),
            size));

    BufferUtilities::copyBuffer(
        commandBuffer,
        staging.getBuffer(),
        0,
        this->_allocation.getBuffer(),
        0,
        size);
  }

  void download(std::vector<TElement>& out) const {
    throw std::runtime_error("NOT IMPLEMENTED");

    // No longer mem-mapped, need to copy to staging buffer and read out...
    // out.resize(this->_structureArray.size());
    // void *pData = this->_allocation.mapMemory();
    // memcpy(
    //     out.data(),
    //     pData,
    //     out.size() * sizeof(TElement));
    // this->_allocation.unmapMemory();
  }

  const TElement& getElement(uint32_t i) const {
    return this->_structureArray[i];
  }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

  size_t getCount() const { return this->_structureArray.size(); }
  size_t getSize() const {
    return this->_structureArray.size() * sizeof(TElement);
  }

private:
  std::vector<TElement> _structureArray;
  BufferAllocation _allocation;
};

} // namespace AltheaEngine