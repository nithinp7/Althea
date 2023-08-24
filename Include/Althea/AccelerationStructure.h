#pragma once

#include "Library.h"
#include "Model.h"
#include "SingleTimeCommandBuffer.h"
#include "UniqueVkHandle.h"
#include "Allocator.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {

class Application;

class ALTHEA_API AccelerationStructure {
public:
  AccelerationStructure(
      Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const std::vector<Model>& models);

private:
  struct AccelerationStructureDeleter {
    void operator()(
        VkDevice device,
        VkAccelerationStructureKHR accelerationStructure);
  };

  UniqueVkHandle<VkAccelerationStructureKHR, AccelerationStructureDeleter> _accelerationStructure;
  BufferAllocation _buffer;
};
} // namespace AltheaEngine