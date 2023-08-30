#include "TextureHeap.h"

#include "Application.h"
#include "Model.h"
#include "Primitive.h"
#include "Texture.h"

// TODO: Implement partial updating of the heap

namespace AltheaEngine {
TextureHeap::TextureHeap(const std::vector<Model>& models)
    : _size(0), _capacity(0) {
  for (const Model& model : models)
    this->_capacity += model.getPrimitivesCount() * 5;

  this->_size = this->_capacity;

  this->_imageInfos.resize(this->_size);
  this->_descriptorWrites.resize(this->_size);

  uint32_t texSlotIdx = 0;
  for (const Model& model : models) {
    for (const Primitive& prim : model.getPrimitives()) {
      const TextureSlots& textures = prim.getTextures();

      this->_initDescriptorWrite(*textures.pBaseTexture, texSlotIdx++);
      this->_initDescriptorWrite(*textures.pNormalMapTexture, texSlotIdx++);
      this->_initDescriptorWrite(
          *textures.pMetallicRoughnessTexture,
          texSlotIdx++);
      this->_initDescriptorWrite(*textures.pOcclusionTexture, texSlotIdx++);
      this->_initDescriptorWrite(*textures.pEmissiveTexture, texSlotIdx++);
    }
  }
}

void TextureHeap::updateSetAndBinding(
    VkDevice device,
    VkDescriptorSet set,
    uint32_t binding) {
  for (VkWriteDescriptorSet& write : this->_descriptorWrites) {
    write.dstSet = set;
    write.dstBinding = binding;
  }

  vkUpdateDescriptorSets(
      device,
      this->_size,
      this->_descriptorWrites.data(),
      0,
      nullptr);
}

void TextureHeap::_initDescriptorWrite(
    const Texture& texture,
    uint32_t slotIdx) {
  VkDescriptorImageInfo& imageInfo = this->_imageInfos[slotIdx];
  VkWriteDescriptorSet& descriptorWrite = this->_descriptorWrites[slotIdx];

  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = texture.getImageView();
  imageInfo.sampler = texture.getSampler();

  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = 0; // set and binding get filled in updateSetBinding
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = nullptr;
  descriptorWrite.pImageInfo = &imageInfo;
  descriptorWrite.pTexelBufferView = nullptr;
}
} // namespace AltheaEngine