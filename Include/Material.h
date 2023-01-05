#pragma once

#include "DescriptorSet.h"
#include "FrameContext.h"
#include "ResourcesAssignment.h"

#include <vector>

namespace AltheaEngine {
class Application;

class Material {
public:
  Material(const Application& app, DescriptorSetAllocator& allocator);
  VkDescriptorSet getCurrentDescriptorSet(const FrameContext& frame) const;
  
  ResourcesAssignment assign();
  
private:
  std::vector<DescriptorSet> _descriptorSets;
};
} // namespace AltheaEngine