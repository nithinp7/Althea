#pragma once

#include "DescriptorSet.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <cassert>

namespace AltheaEngine {
class ResourcesAssignment {
public:
  ResourcesAssignment(std::vector<DescriptorSet>& descriptorSets);
  
  ResourcesAssignment&
  bindTexture(VkImageView imageView, VkSampler sampler);

  // TODO: abstract uniform buffers so client doesn't have to worry about
  // double buffering
  template <typename TUniforms>
  ResourcesAssignment& bindUniformBuffer(
      const std::vector<VkBuffer>& uniformBuffers) {
    assert(uniformBuffers.size() == this->_assignments.size());

    for (uint32_t i = 0; i < this->_assignments.size(); ++i) {
      this->_assignments[i].bindUniformBufferDescriptor<TUniforms>(uniformBuffers[i]);
    }

    return *this;
  }

  template <typename TPrimitiveConstants>
  ResourcesAssignment&
  bindInlineConstants(const TPrimitiveConstants* pConstants) {
    for (DescriptorAssignment& assignment : this->_assignments) {
      assignment.bindInlineConstantDescriptors(pConstants);
    }

    return *this;
  }
  
private:
  std::vector<DescriptorAssignment> _assignments;
};
} // namespace AltheaEngine