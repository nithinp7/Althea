#pragma once

#include "Library.h"

#include "DescriptorSet.h"
#include "Texture.h"
#include "TransientUniforms.h"
#include "UniformBuffer.h"

#include <vulkan/vulkan.h>

#include <cassert>
#include <vector>

namespace AltheaEngine {
class ALTHEA_API ResourcesAssignment {
public:
  ResourcesAssignment(std::vector<DescriptorSet>& descriptorSets);

  ResourcesAssignment& bindTexture(const Texture& texture);

  ResourcesAssignment& bindTexture(VkImageView imageView, VkSampler sampler);

  ResourcesAssignment&
  bindStorageImage(VkImageView imageView, VkSampler sampler);

  template <typename TUniforms>
  ResourcesAssignment&
  bindConstantUniforms(const UniformBuffer<TUniforms>& buffer) {
    for (uint32_t i = 0; i < this->_assignments.size(); ++i) {
      this->_assignments[i].bindUniformBufferDescriptor(buffer);
    }

    return *this;
  }

  // TODO: abstract uniform buffers so client doesn't have to worry about
  // double buffering
  template <typename TUniforms>
  ResourcesAssignment&
  bindTransientUniforms(const TransientUniforms<TUniforms>& buffer) {
    const std::vector<UniformBuffer<TUniforms>>& uniformBuffers =
        buffer.getUniformBuffers();
    assert(uniformBuffers.size() == this->_assignments.size());

    for (uint32_t i = 0; i < this->_assignments.size(); ++i) {
      this->_assignments[i].bindUniformBufferDescriptor(uniformBuffers[i]);
    }

    return *this;
  }

  template <typename TPrimitiveConstants>
  ResourcesAssignment&
  bindInlineConstants(const TPrimitiveConstants& constants) {
    for (DescriptorAssignment& assignment : this->_assignments) {
      assignment.bindInlineConstantDescriptors(constants);
    }

    return *this;
  }

private:
  std::vector<DescriptorAssignment> _assignments;
};
} // namespace AltheaEngine