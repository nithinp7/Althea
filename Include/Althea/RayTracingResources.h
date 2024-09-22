#pragma once

#include "AccelerationStructure.h"
#include "Common/RayTracingCommon.h"
#include "ImageResource.h"

#include <vector>

namespace AltheaEngine {
class Application;
class GlobalHeap;
class SingleTimeCommandBuffer;
class Model;

struct RayTracingResourcesBuilder {
  const std::vector<Model>* models = nullptr;
};

class RayTracingResources {
public:
  RayTracingResources() = default;
  RayTracingResources(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap,
      const RayTracingResourcesBuilder& builder);

  void transitionToTarget(VkCommandBuffer commandBuffer);
  void transitionToTexture(VkCommandBuffer commandBuffer);

  const ImageResource& getTarget() const {
    return m_target;
  }

  const AccelerationStructure& getAccelerationStructure() const {
    return m_accelerationStructure;
  }

  RayTracingHandles getHandles() const;

private:
  ImageResource m_target;
  AccelerationStructure m_accelerationStructure;
};
} // namespace AltheaEngine