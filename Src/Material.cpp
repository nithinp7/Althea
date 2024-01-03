#include "Material.h"

#include "Application.h"

#include <cstdint>

namespace AltheaEngine {
Material::Material(const Application& app, DescriptorSetAllocator& allocator) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    this->_descriptorSets[i] = allocator.allocate();
  }
}

VkDescriptorSet
Material::getCurrentDescriptorSet(const FrameContext& frame) const {
  return this->_descriptorSets[frame.frameRingBufferIndex].getVkDescriptorSet();
}

ResourcesAssignment Material::assign() {
  return ResourcesAssignment(this->_descriptorSets);
}
} // namespace AltheaEngine