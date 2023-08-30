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

  uint32_t texSlotIdx = 0;
  for (const Model& model : models) {
    for (const Primitive& prim : model.getPrimitives()) {
      const TextureSlots& textures = prim.getTextures();

      this->_initImageInfo(*textures.pBaseTexture, texSlotIdx++);
      this->_initImageInfo(*textures.pNormalMapTexture, texSlotIdx++);
      this->_initImageInfo(
          *textures.pMetallicRoughnessTexture,
          texSlotIdx++);
      this->_initImageInfo(*textures.pOcclusionTexture, texSlotIdx++);
      this->_initImageInfo(*textures.pEmissiveTexture, texSlotIdx++);
    }
  }
}

void TextureHeap::_initImageInfo(
    const Texture& texture,
    uint32_t slotIdx) {
  VkDescriptorImageInfo& imageInfo = this->_imageInfos[slotIdx];

  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = texture.getImageView();
  imageInfo.sampler = texture.getSampler();
}
} // namespace AltheaEngine