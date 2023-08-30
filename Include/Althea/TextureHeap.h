#pragma once

#include "DescriptorSet.h"
#include "ImageResource.h"
#include "Library.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace AltheaEngine {
class Application;
class Model;
class Texture;

class TextureHeap {
  friend class DescriptorAssignment;

public:
  TextureHeap() = default;
  TextureHeap(const std::vector<Model>& models);

  uint32_t getSize() const { return this->_size; }
  uint32_t getCaacity() const { return this->_capacity; }

  void
  updateSetAndBinding(VkDevice device, VkDescriptorSet set, uint32_t binding);

private:
  void _initDescriptorWrite(const Texture& texture, uint32_t slotIdx);

  std::vector<VkDescriptorImageInfo> _imageInfos{};
  std::vector<VkWriteDescriptorSet> _descriptorWrites{};

  uint32_t _size;
  uint32_t _capacity;
};
} // namespace AltheaEngine