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
  BufferAllocation _tlasInstances;
  std::vector<VkAccelerationStructureBuildRangeInfoKHR> _blasBuildRanges;
  VkAccelerationStructureBuildRangeInfoKHR* _pBlasBuildRanges; // ???
  std::vector<VkAccelerationStructureBuildRangeInfoKHR> _tlasBuildRanges;
  VkAccelerationStructureBuildRangeInfoKHR* _pTlasBuildRanges; // ???
};
} // namespace AltheaEngine