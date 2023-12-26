#include "PerFrameResources.h"
#include "ResourcesAssignment.h"

#include "Application.h"

namespace AltheaEngine {
PerFrameResources::PerFrameResources(
    const Application& app,
    const DescriptorSetLayoutBuilder& layoutBuilder)
    : _descriptorSetAllocator(
          app,
          layoutBuilder,
          MAX_FRAMES_IN_FLIGHT) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    this->_descriptorSets[i] = this->_descriptorSetAllocator.allocate();
  }
}

VkDescriptorSet
PerFrameResources::getCurrentDescriptorSet(const FrameContext& frame) const {
  return this->_descriptorSets[frame.frameRingBufferIndex].getVkDescriptorSet();
}

ResourcesAssignment PerFrameResources::assign() {
  return ResourcesAssignment(this->_descriptorSets);
}
} // namespace AltheaEngine