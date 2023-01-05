#include "PerFrameResources.h"
#include "ResourcesAssignment.h"

#include "Application.h"

namespace AltheaEngine {
PerFrameResources::PerFrameResources(
    const Application& app,
    const DescriptorSetLayoutBuilder& layoutBuilder)
    : _pDescriptorSetAllocator(std::make_unique<DescriptorSetAllocator>(
          app,
          layoutBuilder,
          app.getMaxFramesInFlight())) {
  this->_descriptorSets.reserve(app.getMaxFramesInFlight());
  for (uint32_t i = 0; i < app.getMaxFramesInFlight(); ++i) {
    this->_descriptorSets.emplace_back(
        this->_pDescriptorSetAllocator->allocate());
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