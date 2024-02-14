#pragma once

#include "DescriptorSet.h"
#include "FrameContext.h"
#include "Library.h"

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

  PerFrameResources(PerFrameResources&& other) {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
      this->_descriptorSets[i] = {};
    this->_descriptorSetAllocator = std::move(other._descriptorSetAllocator);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      this->_descriptorSets[i] = std::move(other._descriptorSets[i]);
      // need to fix up the allocator pointer FFS
      this->_descriptorSets[i]._allocator = &this->_descriptorSetAllocator;
    }
  }

  PerFrameResources& operator=(PerFrameResources&& other) {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
      this->_descriptorSets[i] = {};
    this->_descriptorSetAllocator = std::move(other._descriptorSetAllocator);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      this->_descriptorSets[i] = std::move(other._descriptorSets[i]);
      // need to fix up the allocator pointer FFS
      this->_descriptorSets[i]._allocator = &this->_descriptorSetAllocator;
    }
    
    return *this;
  }

  VkDescriptorSetLayout getLayout() const {
    return this->_descriptorSetAllocator.getLayout();
  }

  VkDescriptorSet getCurrentDescriptorSet(const FrameContext& frame) const;

  ResourcesAssignment assign();

private:
  DescriptorSetAllocator _descriptorSetAllocator;
  // One descriptor set for each frame-in-flight
  DescriptorSet _descriptorSets[MAX_FRAMES_IN_FLIGHT]{};
};
} // namespace AltheaEngine