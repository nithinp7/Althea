#pragma once

#include "Allocator.h"
#include "BindlessHandle.h"
#include "BufferUtilities.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;
class GlobalHeap;

template <typename TConstants> class ALTHEA_API ConstantBuffer {
public:
  ConstantBuffer() = default;

  ConstantBuffer(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const TConstants& constants)
      : _constants(constants) {

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.flags = 0;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VkBuffer staging = commandBuffer.createStagingBuffer(
        app,
        gsl::span((const std::byte*)&this->_constants, sizeof(TConstants)));

    this->_allocation = BufferUtilities::createBuffer(
        app,
        sizeof(TConstants),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        allocInfo);

    BufferUtilities::copyBuffer(
        commandBuffer,
        staging,
        0,
        this->_allocation.getBuffer(),
        0,
        sizeof(TConstants));
  }

  void registerToHeap(GlobalHeap& heap) {
    this->_handle = heap.registerBuffer();
    heap.updateStorageBuffer(
        this->_handle,
        this->_allocation.getBuffer(),
        0,
        this->getSize());
  }

  const TConstants& getConstants() const { return this->_constants; }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

  size_t getSize() const { return sizeof(TConstants); }

  BufferHandle getHandle() const { return this->_handle; }

private:
  void _createUniformBuffer(const Application& app) {}

  TConstants _constants;
  BufferAllocation _allocation;
  BufferHandle _handle;
};

} // namespace AltheaEngine