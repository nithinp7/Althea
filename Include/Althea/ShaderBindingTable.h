#pragma once

#include "Allocator.h"
#include "Library.h"

#include <vulkan/vulkan.h>

namespace AltheaEngine {
class Application;
class RayTracingPipeline;

class ALTHEA_API ShaderBindingTable {
public:
  ShaderBindingTable() = default;
  ShaderBindingTable(
      const Application& app,
      const RayTracingPipeline& pipeline);

  VkBuffer getBuffer() const { return this->_sbt.getBuffer(); }

  const VkStridedDeviceAddressRegionKHR& getRayGenRegion() const { return this->_rgenRegion; }
  const VkStridedDeviceAddressRegionKHR& getMissRegion() const { return this->_missRegion; }
  const VkStridedDeviceAddressRegionKHR& getHitRegion() const { return this->_hitRegion; }
  const VkStridedDeviceAddressRegionKHR& getCallRegion() const { return this->_callRegion; }

private:
  BufferAllocation _sbt;
  VkStridedDeviceAddressRegionKHR _rgenRegion{};
  VkStridedDeviceAddressRegionKHR _missRegion{};
  VkStridedDeviceAddressRegionKHR _hitRegion{};
  VkStridedDeviceAddressRegionKHR _callRegion{};
};
} // namespace AltheaEngine