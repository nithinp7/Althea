#pragma once

#include "Library.h"
#include "Model.h"
#include "SingleTimeCommandBuffer.h"
#include "UniqueVkHandle.h"
#include "Allocator.h"
#include "UniformBuffer.h"
#include "BindlessHandle.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {

class Application;
class GlobalHeap;

class ALTHEA_API AccelerationStructure {
public:
  AccelerationStructure() = default;
  AccelerationStructure(
      Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const std::vector<Model>& models);

  void registerToHeap(GlobalHeap& heap);

  VkAccelerationStructureKHR getTLAS() const {
    return this->_tlas;
  }

  VkAccelerationStructureKHR getBLAS(uint32_t i) const {
    return this->_blasCollection[i];
  }

  TlasHandle getTlasHandle() const { return _tlasHandle; }

private:
  struct AccelerationStructureDeleter {
    void operator()(
        VkDevice device,
        VkAccelerationStructureKHR accelerationStructure);
  };

  UniqueVkHandle<VkAccelerationStructureKHR, AccelerationStructureDeleter> _tlas;
  std::vector<UniqueVkHandle<VkAccelerationStructureKHR, AccelerationStructureDeleter>> _blasCollection;
  BufferAllocation _tlasBuffer;
  std::vector<BufferAllocation> _blasBuffers;
  BufferAllocation _transformBuffer;
  BufferAllocation _tlasInstances;

  TlasHandle _tlasHandle;
};
} // namespace AltheaEngine