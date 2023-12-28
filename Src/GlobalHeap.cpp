#include "GlobalHeap.h"

#include "Application.h"

#include <stdexcept>

#define STORAGE_BUFFER_HEAP_BINDING 0
#define UNIFORM_HEAP_BINDING 1
#define TEXTURE_HEAP_BINDING 2

namespace AltheaEngine {
GlobalHeap::GlobalHeap(const Application& app)
: _device(app.getDevice()) {
  DescriptorSetLayoutBuilder layoutBuilder{};
  layoutBuilder.addBufferHeapBinding(BUFFER_HEAP_SLOTS, VK_SHADER_STAGE_ALL);
  layoutBuilder.addUniformHeapBinding(UNIFORM_HEAP_SLOTS, VK_SHADER_STAGE_ALL);
  layoutBuilder.addTextureHeapBinding(TEXTURE_HEAP_SLOTS, VK_SHADER_STAGE_ALL);

  this->_setAllocator = DescriptorSetAllocator(app, layoutBuilder, 1);
  this->_set = this->_setAllocator.allocateVkDescriptorSet();
}

void GlobalHeap::updateStorageBuffer(
    BufferHandle handle,
    VkBuffer buffer,
    size_t offset,
    size_t size) {
  if (!handle.isValid()) {
    throw std::runtime_error(
        "Attempting to update bindless slot using invalid handle!");
  }

  VkDescriptorBufferInfo bufferInfo;
  bufferInfo.buffer = buffer;
  bufferInfo.offset = offset;
  bufferInfo.range = size;

  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = this->_set;
  write.dstBinding = STORAGE_BUFFER_HEAP_BINDING;
  write.dstArrayElement = handle.index;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write.descriptorCount = 1;
  write.pBufferInfo = &bufferInfo;
  write.pImageInfo = nullptr;
  write.pTexelBufferView = nullptr;

  vkUpdateDescriptorSets(this->_device, 1, &write, 0, nullptr);
}

void GlobalHeap::updateUniformBuffer(
    UniformHandle handle,
    VkBuffer buffer,
    size_t offset,
    size_t size) {
  if (!handle.isValid()) {
    throw std::runtime_error(
        "Attempting to update bindless slot using invalid handle!");
  }

  VkDescriptorBufferInfo bufferInfo;
  bufferInfo.buffer = buffer;
  bufferInfo.offset = offset;
  bufferInfo.range = size;

  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = this->_set;
  write.dstBinding = UNIFORM_HEAP_BINDING;
  write.dstArrayElement = handle.index;
  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.descriptorCount = 1;
  write.pBufferInfo = &bufferInfo;
  write.pImageInfo = nullptr;
  write.pTexelBufferView = nullptr;
  
  vkUpdateDescriptorSets(this->_device, 1, &write, 0, nullptr);
}

void GlobalHeap::updateTexture(
    ImageHandle handle,
    VkImageView view,
    VkSampler sampler) {
  if (!handle.isValid()) {
    throw std::runtime_error(
        "Attempting to update bindless slot using invalid handle!");
  }

  VkDescriptorImageInfo imageInfo;
  imageInfo.imageView = view;
  imageInfo.sampler = sampler;
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = this->_set;
  write.dstBinding = TEXTURE_HEAP_BINDING;
  write.dstArrayElement = handle.index;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.descriptorCount = 1;
  write.pBufferInfo = nullptr;
  write.pImageInfo = &imageInfo;
  write.pTexelBufferView = nullptr;
  
  vkUpdateDescriptorSets(this->_device, 1, &write, 0, nullptr);
}

void GlobalHeap::updateStorageImage(
    ImageHandle handle,
    VkImageView view,
    VkSampler sampler) {
  if (!handle.isValid()) {
    throw std::runtime_error(
        "Attempting to update bindless slot using invalid handle!");
  }

  VkDescriptorImageInfo imageInfo;
  imageInfo.imageView = view;
  imageInfo.sampler = sampler;
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = this->_set;
  write.dstBinding = TEXTURE_HEAP_BINDING;
  write.dstArrayElement = handle.index;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  write.descriptorCount = 1;
  write.pBufferInfo = nullptr;
  write.pImageInfo = &imageInfo;
  write.pTexelBufferView = nullptr;
  
  vkUpdateDescriptorSets(this->_device, 1, &write, 0, nullptr);
}
} // namespace AltheaEngine