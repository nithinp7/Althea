#include "IndexBuffer.h"

#include "Application.h"
#include "BufferUtilities.h"

#include <gsl/span>

namespace AltheaEngine {
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
  deviceAllocInfo.flags = 0;
  deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  this->_allocation = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      indicesView.size(),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      deviceAllocInfo);

  BufferUtilities::copyBuffer(
      commandBuffer,
      stagingBuffer,
      0,
      this->_allocation.getBuffer(),
      0,
      indicesView.size());
}
} // namespace AltheaEngine