#include "BufferUtilities.h"

#include "Application.h"

#include <vulkan/vulkan.h>

namespace AltheaEngine {
/*static*/
void BufferUtilities::copyBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    size_t srcOffset,
    VkBuffer dstBuffer,
    size_t dstOffset,
    size_t size) {

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = srcOffset;
  copyRegion.dstOffset = dstOffset;
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

/*static*/
BufferAllocation BufferUtilities::createBuffer(
    const Application& app,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    const VmaAllocationCreateInfo& allocInfo) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  return app.getAllocator().createBuffer(bufferInfo, allocInfo);
}

/*static*/
BufferAllocation BufferUtilities::createStagingBuffer(
    const Application& app,
    size_t bufferSize) {
  VmaAllocationCreateInfo stagingInfo{};
  stagingInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  stagingInfo.usage = VMA_MEMORY_USAGE_AUTO;

  return createBuffer(
      app,
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      stagingInfo);
}

/*static*/
BufferAllocation BufferUtilities::createStagingBuffer(
    const Application& app,
    gsl::span<const std::byte> srcBuffer) {
  BufferAllocation stagingBuffer =
      createStagingBuffer(app, srcBuffer.size());

  void* data = stagingBuffer.mapMemory();
  memcpy(data, srcBuffer.data(), srcBuffer.size());
  stagingBuffer.unmapMemory();

  return stagingBuffer;
}
} // namespace AltheaEngine