#pragma once

#include "DescriptorSet.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {
class Application;

#define INVALID_BINDLESS_HANDLE 0xFFFFFFFF
#define BUFFER_HEAP_SLOTS 1024
#define TEXTURE_HEAP_SLOTS 1024

struct BufferHandle {
  uint32_t index = INVALID_BINDLESS_HANDLE;

  bool isValid() const { return index != INVALID_BINDLESS_HANDLE; }
};

struct ImageHandle {
  uint32_t index = INVALID_BINDLESS_HANDLE;

  bool isValid() const { return index != INVALID_BINDLESS_HANDLE; }
};

class GlobalHeap {
public:
  GlobalHeap() = default;
  GlobalHeap(const Application& app);

  BufferHandle allocateBuffer() { return {this->_usedBufferSlots++}; };
  ImageHandle allocaeTexture() { return {this->_usedTextureSlots++}; };

  void updateStorageBuffer(
      BufferHandle handle,
      VkBuffer buffer,
      size_t offset,
      size_t size);
  void updateUniformBuffer(
      BufferHandle handle,
      VkBuffer buffer,
      size_t offset,
      size_t size);
  void updateTexture(
      ImageHandle handle,
      VkImageView view,
      VkSampler sampler);
  void updateStorageImage(
      ImageHandle handle,
      VkImageView view,
      VkSampler sampler);

private:
  VkDevice _device;
  
  DescriptorSetAllocator _setAllocator;
  VkDescriptorSet _set;

  std::vector<VkDescriptorBufferInfo> _bufferInfos{};
  std::vector<VkDescriptorImageInfo> _imageInfos{};

  uint32_t _usedBufferSlots = 0;
  uint32_t _usedTextureSlots = 0;
};
} // namespace AltheaEngine