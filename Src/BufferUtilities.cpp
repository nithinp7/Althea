
#include "Application.h"

#include <vulkan/vulkan.h>

#include <stdexcept>

// TODO: Refactor most of this into the allocator class!!
// TODO: Do another pass to abstract boiler plate here.

namespace AltheaEngine {
// Buffer related implementations for the Application class
uint32_t Application::findMemoryType(
    uint32_t typeFilter,
    const VkMemoryPropertyFlags& properties) const {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");

  return 0;
}

void Application::copyBuffer(
    const VkBuffer& srcBuffer,
    const VkBuffer& dstBuffer,
    VkDeviceSize size) const {

  VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  this->endSingleTimeCommands(commandBuffer);
}

BufferAllocation Application::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    const VmaAllocationCreateInfo& allocInfo) const {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  return this->pAllocator->createBuffer(bufferInfo, allocInfo);
}

BufferAllocation Application::createVertexBuffer(
    const void* pSrc,
    VkDeviceSize bufferSize) const {

  VmaAllocationCreateInfo stagingInfo{};
  stagingInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  stagingInfo.usage = VMA_MEMORY_USAGE_AUTO;

  BufferAllocation stagingBuffer =
      createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingInfo);

  void* data = stagingBuffer.mapMemory();
  memcpy(data, pSrc, (size_t)bufferSize);
  stagingBuffer.unmapMemory();

  VmaAllocationCreateInfo deviceAllocInfo{};
  deviceAllocInfo.flags = 0;
  deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  BufferAllocation vertexBuffer = createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      deviceAllocInfo);

  copyBuffer(stagingBuffer.getBuffer(), vertexBuffer.getBuffer(), bufferSize);

  return vertexBuffer;
}

BufferAllocation Application::createIndexBuffer(
    const void* pSrc,
    VkDeviceSize bufferSize) const {

  VmaAllocationCreateInfo stagingInfo{};
  stagingInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  stagingInfo.usage = VMA_MEMORY_USAGE_AUTO;

  BufferAllocation stagingBuffer =
      createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingInfo);

  void* data = stagingBuffer.mapMemory();
  memcpy(data, pSrc, (size_t)bufferSize);
  stagingBuffer.unmapMemory();

  VmaAllocationCreateInfo deviceAllocInfo{};
  deviceAllocInfo.flags = 0;
  deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  BufferAllocation indexBuffer = createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      deviceAllocInfo);

  copyBuffer(stagingBuffer.getBuffer(), indexBuffer.getBuffer(), bufferSize);

  return indexBuffer;
}

BufferAllocation
Application::createUniformBuffer(VkDeviceSize bufferSize) const {

  // TODO: This assumes that the uniform buffer will be _often_ rewritten
  // and perhaps in a random pattern. We should prefer a different type of
  // memory if the uniform buffer will mostly be persistent.
  VmaAllocationCreateInfo allocInfo{};
  allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  return createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      allocInfo);
}
} // namespace AltheaEngine