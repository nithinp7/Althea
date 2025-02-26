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
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    const VmaAllocationCreateInfo& allocInfo) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  return GAllocator->createBuffer(bufferInfo, allocInfo);
}

/*static*/
BufferAllocation BufferUtilities::createStagingBuffer(size_t bufferSize) {
  VmaAllocationCreateInfo stagingInfo{};
  stagingInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  stagingInfo.usage = VMA_MEMORY_USAGE_AUTO;

  return createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      stagingInfo);
}

/*static*/
BufferAllocation BufferUtilities::createStagingBufferForDownload(size_t bufferSize) {
  VmaAllocationCreateInfo stagingInfo{};
  stagingInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  stagingInfo.usage = VMA_MEMORY_USAGE_AUTO;

  return createBuffer(
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    stagingInfo);
}

/*static*/
BufferAllocation
BufferUtilities::createStagingBuffer(gsl::span<const std::byte> srcBuffer) {
  BufferAllocation stagingBuffer = createStagingBuffer(srcBuffer.size());

  void* data = stagingBuffer.mapMemory();
  memcpy(data, srcBuffer.data(), srcBuffer.size());
  stagingBuffer.unmapMemory();

  return stagingBuffer;
}

// just for backward compat
/*static*/
BufferAllocation BufferUtilities::createBuffer(
    const Application& app,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    const VmaAllocationCreateInfo& allocInfo) {
  return createBuffer(size, usage, allocInfo);
}

/*static*/
BufferAllocation BufferUtilities::createStagingBuffer(
    const Application& app,
    size_t bufferSize) {
  return createStagingBuffer(bufferSize);
}

/*static*/
BufferAllocation BufferUtilities::createStagingBuffer(
    const Application& app,
    gsl::span<const std::byte> srcBuffer) {
  return createStagingBuffer(srcBuffer);
}

/*static*/
void BufferUtilities::rwBarrier(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    size_t offset,
    size_t size) {
  VkBufferMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.buffer = buffer;
  barrier.offset = offset;
  barrier.size = size;
  barrier.srcAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
      VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
      0,
      0,
      nullptr,
      1,
      &barrier,
      0,
      nullptr);
}
} // namespace AltheaEngine