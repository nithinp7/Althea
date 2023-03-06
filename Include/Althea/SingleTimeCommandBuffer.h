#pragma once

#include "Library.h"
#include "Allocator.h"

#include <vulkan/vulkan.h>
#include <gsl/span>

#include <vector>
#include <memory>
#include <functional>

namespace AltheaEngine {
class Application;

class ALTHEA_API SingleTimeCommandBuffer {
public:
  SingleTimeCommandBuffer(const Application& app);
  ~SingleTimeCommandBuffer();

  VkBuffer createStagingBuffer(const Application& app, gsl::span<const std::byte> buffer);

  void addPostCompletionTask(std::function<void()>&& task);

  operator VkCommandBuffer() const {
    return this->_commandBuffer;
  }

private:
  VkDevice _device;
  VkQueue _queue;
  VkCommandPool _commandPool;
  VkCommandBuffer _commandBuffer;
  std::vector<std::function<void()>> _postCompletionTasks;
  std::vector<BufferAllocation> _stagingBuffers;
};
} // namespace AltheaEngine