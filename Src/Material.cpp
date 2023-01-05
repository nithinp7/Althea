#include "Material.h"
#include "Application.h"

#include <cstdint>

namespace AltheaEngine {
Material::Material(const Application& app, DescriptorSetAllocator& allocator) {
  this->_descriptorSets.reserve(app.getMaxFramesInFlight());
  for (uint32_t i = 0; i < app.getMaxFramesInFlight(); ++i) {
    this->_descriptorSets.emplace_back(allocator.allocate());
  }
}

VkDescriptorSet Material::getCurrentDescriptorSet(const FrameContext& frame) const {
  return this->_descriptorSets[frame.frameRingBufferIndex].getVkDescriptorSet();
}

ResourcesAssignment Material::assign() {
  return ResourcesAssignment(this->_descriptorSets);
}
} // namespace AltheaEngine