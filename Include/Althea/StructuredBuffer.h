#pragma once

#include "Allocator.h"
#include "BufferUtilities.h"
#include "GlobalHeap.h"
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

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    this->_allocation = BufferUtilities::createBuffer(
        app,
        sizeof(TElement) * count,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            additionalFlags,
        allocInfo);
  }

  void registerToHeap(GlobalHeap& heap) {
    this->_handle = heap.registerBuffer();
    heap.updateStorageBuffer(
        this->_handle,
        this->_allocation.getBuffer(),
        0,
        getSize());
  }

  void setElement(const TElement& element, uint32_t i) {
    this->_structureArray[i] = element;
  }

  void upload(Application& app, VkCommandBuffer commandBuffer) const {
    size_t size = this->getSize();
    gsl::span<const std::byte> view(
        reinterpret_cast<const std::byte*>(this->_structureArray.data()),
        size);

    BufferAllocation* pStagingAllocation =
        new BufferAllocation(BufferUtilities::createStagingBuffer(app, view));

    BufferUtilities::copyBuffer(
        commandBuffer,
        pStagingAllocation->getBuffer(),
        0,
        this->_allocation.getBuffer(),
        0,
        size);

    // Delete staging buffer allocation once the transfer is complete
    app.addDeletiontask(
        {[pStagingAllocation]() { delete pStagingAllocation; },
         app.getCurrentFrameRingBufferIndex()});
  }

  void rwBarrier(VkCommandBuffer commandBuffer) const {
    BufferUtilities::rwBarrier(
        commandBuffer,
        _allocation.getBuffer(),
        0,
        getSize());
  }

  void barrier(
      VkCommandBuffer commandBuffer,
      VkAccessFlags dstAccessFlags,
      VkPipelineStageFlags dstPipelineStageFlags) const {
    BufferUtilities::barrier(
        commandBuffer,
        dstAccessFlags,
        dstPipelineStageFlags,
        _allocation.getBuffer(),
        0,
        getSize());
  }

  void zeroBuffer(VkCommandBuffer commandBuffer) const {
    vkCmdFillBuffer(commandBuffer, _allocation.getBuffer(), 0, getSize(), 0);
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

  BufferHandle getHandle() const { return this->_handle; }

private:
  std::vector<TElement> _structureArray;
  BufferAllocation _allocation;
  BufferHandle _handle;
};

} // namespace AltheaEngine