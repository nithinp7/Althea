#pragma once

#include "DescriptorSet.h"
#include "ImageResource.h"
#include "Library.h"
#include "Texture.h"
#include "TransientUniforms.h"
#include "UniformBuffer.h"
#include "DynamicBuffer.h"

#include <vulkan/vulkan.h>

#include <cassert>
#include <vector>

namespace AltheaEngine {
class ALTHEA_API ResourcesAssignment {
public:
  ResourcesAssignment(DescriptorSet (&descriptorSets)[MAX_FRAMES_IN_FLIGHT]);
  ResourcesAssignment(std::vector<DescriptorSet>& descriptorSets);

  ResourcesAssignment&
  bindAccelerationStructure(VkAccelerationStructureKHR accelerationStructure);

  ResourcesAssignment& bindTexture(const Texture& texture);

  ResourcesAssignment& bindTexture(const ImageResource& texture);

  ResourcesAssignment& bindTexture(VkImageView imageView, VkSampler sampler);

  template <typename TBufferHeap>
  ResourcesAssignment& bindBufferHeap(TBufferHeap& bufferHeap) {
    for (DescriptorAssignment& assignment : this->_assignments) {
      assignment.bindBufferHeap(bufferHeap);
    }

    return *this;
  }

  ResourcesAssignment&
  bindStorageImage(VkImageView imageView, VkSampler sampler);

  ResourcesAssignment& bindStorageBuffer(
      const BufferAllocation& allocation,
      size_t bufferSize,
      bool doubleBuffered);

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
    for (uint32_t i = 0; i < this->_assignments.size(); ++i) {
      this->_assignments[i].bindUniformBufferDescriptor(
          buffer.getUniformBuffers()[i]);
    }

    return *this;
  }

  ResourcesAssignment& bindTransientUniforms(const DynamicBuffer& buffer) {
    for (uint32_t i = 0; i < this->_assignments.size(); ++i) {
      this->_assignments[i].bindUniformBufferDescriptor(
          buffer.getAllocation().getBuffer(),
          buffer.getSize() * i,
          buffer.getSize());
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