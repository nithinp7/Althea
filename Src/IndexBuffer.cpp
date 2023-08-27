#include "IndexBuffer.h"

#include "Application.h"
#include "BufferUtilities.h"

#include <gsl/span>

namespace AltheaEngine {
// In this constructor, the client is responsible for keeping the srcBuffer
// alive until the copy is complete.
IndexBuffer::IndexBuffer(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    size_t bufferOffset,
    size_t bufferSize) {
  // TODO: ...this is a hack so that indexCount can be computed
  // This should be refactored so indices do NOT have to be locally stored.
  this->_indices.resize(bufferSize / sizeof(uint32_t));

  VmaAllocationCreateInfo deviceAllocInfo{};
  deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  this->_allocation = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
      deviceAllocInfo);

  BufferUtilities::copyBuffer(
      commandBuffer,
      srcBuffer,
      bufferOffset,
      this->_allocation.getBuffer(),
      0,
      bufferSize);
}

IndexBuffer::IndexBuffer(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    std::vector<uint32_t>&& indices)
    : _indices(std::move(indices)) {
  gsl::span<const std::byte> indicesView(
      reinterpret_cast<const std::byte*>(this->_indices.data()),
      sizeof(uint32_t) * this->_indices.size());

  VkBuffer stagingBuffer = commandBuffer.createStagingBuffer(app, indicesView);

  VmaAllocationCreateInfo deviceAllocInfo{};
  deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  this->_allocation = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      indicesView.size(),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
      deviceAllocInfo);

  BufferUtilities::copyBuffer(
      commandBuffer,
      stagingBuffer,
      0,
      this->_allocation.getBuffer(),
      0,
      indicesView.size());
}

IndexBuffer::IndexBuffer(
    Application& app,
    VkCommandBuffer commandBuffer,
    std::vector<uint32_t>&& indices)
    : _indices(std::move(indices)) {
  gsl::span<const std::byte> indicesView(
      reinterpret_cast<const std::byte*>(this->_indices.data()),
      sizeof(uint32_t) * this->_indices.size());

  BufferAllocation* pStagingAllocation = new BufferAllocation(
      BufferUtilities::createStagingBuffer(app, commandBuffer, indicesView));

  VmaAllocationCreateInfo deviceAllocInfo{};
  deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  this->_allocation = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      indicesView.size(),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
      deviceAllocInfo);

  BufferUtilities::copyBuffer(
      commandBuffer,
      pStagingAllocation->getBuffer(),
      0,
      this->_allocation.getBuffer(),
      0,
      indicesView.size());

  // Delete staging buffer allocation once the transfer is complete
  app.addDeletiontask(
      {[pStagingAllocation]() { delete pStagingAllocation; },
       app.getCurrentFrameRingBufferIndex()});
}
} // namespace AltheaEngine