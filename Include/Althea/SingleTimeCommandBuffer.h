#pragma once

#include "Library.h"
#include "Allocator.h"

#include <vulkan/vulkan.h>
#include <gsl/span>

#include <vector>
#include <memory>

namespace AltheaEngine {
class Application;

class ALTHEA_API SingleTimeCommandBuffer {
public:
  SingleTimeCommandBuffer(const Application& app);
  ~SingleTimeCommandBuffer();

  VkBuffer createStagingBuffer(const Application& app, gsl::span<const std::byte> buffer);


  operator VkCommandBuffer() const {
    return this->_commandBuffer;
  }

private:
  VkDevice _device;
  VkQueue _queue;
  VkCommandPool _commandPool;
  VkCommandBuffer _commandBuffer;
  std::vector<BufferAllocation> _stagingBuffers;
};
} // namespace AltheaEngine