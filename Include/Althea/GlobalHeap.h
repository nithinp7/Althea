#pragma once

#include "DescriptorSet.h"
#include "BindlessHandle.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {
class Application;

#define BUFFER_HEAP_SLOTS 1024
#define UNIFORM_HEAP_SLOTS 1024
#define TEXTURE_HEAP_SLOTS 1024

class GlobalHeap {
public:
  GlobalHeap() = default;
  GlobalHeap(const Application& app);

  BufferHandle registerBuffer() { return {this->_usedBufferSlots++}; };
  UniformHandle registerUniforms() { return {this->_usedUniformSlots++}; };
  ImageHandle registerTexture() { return {this->_usedTextureSlots++}; };

  void updateStorageBuffer(
      BufferHandle handle,
      VkBuffer buffer,
      size_t offset,
      size_t size);
  void updateUniformBuffer(
      UniformHandle handle,
      VkBuffer buffer,
      size_t offset,
      size_t size);
  void updateTexture(ImageHandle handle, VkImageView view, VkSampler sampler);
  void
  updateStorageImage(ImageHandle handle, VkImageView view, VkSampler sampler);

  VkDescriptorSet getDescriptorSet() const { return this->_set; }

  VkDescriptorSetLayout getDescriptorSetLayout() const {
    return this->_setAllocator.getLayout();
  }

private:
  VkDevice _device;

  DescriptorSetAllocator _setAllocator;
  VkDescriptorSet _set;

  uint32_t _usedBufferSlots = 0;
  uint32_t _usedUniformSlots = 0;
  uint32_t _usedTextureSlots = 0;
};
} // namespace AltheaEngine