#include "ResourcesAssignment.h"

namespace AltheaEngine {

ResourcesAssignment::ResourcesAssignment(
    std::vector<DescriptorSet>& descriptorSets) {
  this->_assignments.reserve(descriptorSets.size());
  for (DescriptorSet& descriptorSet : descriptorSets) {
    this->_assignments.emplace_back(descriptorSet.assign());
  }
}

ResourcesAssignment& ResourcesAssignment::bindTexture(const Texture& texture) {
  return this->bindTexture(texture.getImageView(), texture.getSampler());
}

ResourcesAssignment& ResourcesAssignment::bindTexture(const ImageResource& texture) {
  return this->bindTexture(texture.view, texture.sampler);
}

ResourcesAssignment&
ResourcesAssignment::bindTexture(VkImageView imageView, VkSampler sampler) {
  for (DescriptorAssignment& assignment : this->_assignments) {
    assignment.bindTextureDescriptor(imageView, sampler);
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

} // namespace AltheaEngine