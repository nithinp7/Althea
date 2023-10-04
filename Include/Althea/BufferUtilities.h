#pragma once

#include "Allocator.h"
#include "Library.h"

#include <vulkan/vulkan.h>
#include <gsl/span>

#include <cstdint>
#include <memory>

namespace AltheaEngine {
class Application;

class ALTHEA_API BufferUtilities {
public:
  static void copyBuffer(
      VkCommandBuffer commandBuffer,
      VkBuffer srcBuffer,
      size_t srcOffset,
      VkBuffer dstBuffer,
      size_t dstOffset,
      size_t size);
  static BufferAllocation createBuffer(
      const Application& app,
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      const VmaAllocationCreateInfo& allocInfo);
  static BufferAllocation createStagingBuffer(
      const Application& app,
      size_t bufferSize);
  static BufferAllocation createStagingBuffer(
      const Application& app,
      gsl::span<const std::byte> srcBuffer);
};
} // namespace AltheaEngine