#pragma once

#include "Library.h"
#include "Model.h"
#include "SingleTimeCommandBuffer.h"
#include "UniqueVkHandle.h"
#include "Allocator.h"
#include "UniformBuffer.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {

class Application;

class ALTHEA_API AccelerationStructure {
public:
  AccelerationStructure() = default;
  AccelerationStructure(
      Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const std::vector<Model>& models);

  VkAccelerationStructureKHR getTLAS() const {
    return this->_tlas;
  }

  VkAccelerationStructureKHR getBLAS() const {
    return this->_blas;
  }

private:
  struct AccelerationStructureDeleter {
    void operator()(
        VkDevice device,
        VkAccelerationStructureKHR accelerationStructure);
  };

  UniqueVkHandle<VkAccelerationStructureKHR, AccelerationStructureDeleter> _tlas;
  UniqueVkHandle<VkAccelerationStructureKHR, AccelerationStructureDeleter> _blas;
  BufferAllocation _tlasBuffer;
  BufferAllocation _blasBuffer;
  BufferAllocation _transformBuffer;
  BufferAllocation _tlasInstances;
  std::vector<VkAccelerationStructureBuildRangeInfoKHR> _blasBuildRanges;
  std::vector<VkAccelerationStructureBuildRangeInfoKHR> _tlasBuildRanges;
  // Note: These pointers are not heap-allocated and they do not need to be freed.
  // They are simply pointers to the above buildRanges, we need this bc we need
  // pointers to pointers to build ranges (I have no clue why)
  VkAccelerationStructureBuildRangeInfoKHR* _pBlasBuildRanges; // ???
  VkAccelerationStructureBuildRangeInfoKHR* _pTlasBuildRanges; // ???
};
} // namespace AltheaEngine