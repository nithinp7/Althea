#pragma once

#include "Library.h"

#include "DescriptorSet.h"
#include "FrameContext.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {
class Application;
class ResourcesAssignment;

class ALTHEA_API PerFrameResources {
public:
  PerFrameResources() = default;
  PerFrameResources(
      const Application& app,
      const DescriptorSetLayoutBuilder& layoutBuilder);

  VkDescriptorSetLayout getLayout() const {
    return this->_pDescriptorSetAllocator->getLayout();
  }

  VkDescriptorSet getCurrentDescriptorSet(const FrameContext& frame) const;

  ResourcesAssignment assign();

private:
  // TODO: Does this still need to be in a unique_ptr
  std::unique_ptr<DescriptorSetAllocator> _pDescriptorSetAllocator; 

  // One descriptor set for each frame-in-flight
  std::vector<DescriptorSet> _descriptorSets;
};
} // namespace AltheaEngine