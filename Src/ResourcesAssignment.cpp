#include "ResourcesAssignment.h"

namespace AltheaEngine {

ResourcesAssignment::ResourcesAssignment(
    std::vector<DescriptorSet>& descriptorSets) {
  this->_assignments.reserve(descriptorSets.size());
  for (DescriptorSet& descriptorSet : descriptorSets) {
    this->_assignments.emplace_back(descriptorSet.assign());
  }
}

ResourcesAssignment& ResourcesAssignment::bindAccelerationStructure(
    VkAccelerationStructureKHR accelerationStructure) {
  for (DescriptorAssignment& assignment : this->_assignments) {
    assignment.bindAccelerationStructure(accelerationStructure);
  }

  return *this;
}

ResourcesAssignment& ResourcesAssignment::bindTexture(const Texture& texture) {
  return this->bindTexture(texture.getImageView(), texture.getSampler());
}

ResourcesAssignment&
ResourcesAssignment::bindTexture(const ImageResource& texture) {
  return this->bindTexture(texture.view, texture.sampler);
}

ResourcesAssignment&
ResourcesAssignment::bindTexture(VkImageView imageView, VkSampler sampler) {
  for (DescriptorAssignment& assignment : this->_assignments) {
    assignment.bindTextureDescriptor(imageView, sampler);
  }

  return *this;
}

ResourcesAssignment&
ResourcesAssignment::bindTextureHeap(TextureHeap& textureHeap) {
  for (DescriptorAssignment& assignment : this->_assignments) {
    assignment.bindTextureHeap(textureHeap);
  }

  return *this;
}

ResourcesAssignment& ResourcesAssignment::bindStorageImage(
    VkImageView imageView,
    VkSampler sampler) {
  for (DescriptorAssignment& assignment : this->_assignments) {
    assignment.bindStorageImage(imageView, sampler);
  }

  return *this;
}

ResourcesAssignment& ResourcesAssignment::bindStorageBuffer(
    const BufferAllocation& allocation,
    size_t bufferSize,
    bool doubleBuffered) {
  for (size_t ringBufferIndex = 0; ringBufferIndex < this->_assignments.size();
       ++ringBufferIndex) {
    DescriptorAssignment& assignment = this->_assignments[ringBufferIndex];

    if (doubleBuffered) {
      // If the storage buffer is double buffered, then it consists of several
      // subsets each of `bufferSize` length. Each of these subsets will
      // correspond to one frame ring buffer index and thus one of the
      // "assignments".
      assignment.bindStorageBuffer(
          allocation,
          ringBufferIndex * bufferSize,
          bufferSize);
    } else {
      // If it is not double buffered, then there is a single storage buffer
      // that is used regardless of the frame ring buffer index.
      assignment.bindStorageBuffer(allocation, 0, bufferSize);
    }
  }

  return *this;
}
} // namespace AltheaEngine