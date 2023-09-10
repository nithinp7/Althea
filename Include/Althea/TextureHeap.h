#pragma once

#include "DescriptorSet.h"
#include "ImageResource.h"
#include "Library.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace AltheaEngine {
class Application;
class Model;

class TextureHeap {
  friend class DescriptorAssignment;

public:
  TextureHeap() = default;
  TextureHeap(const std::vector<Model>& models);

  uint32_t getSize() const { return this->_size; }
  uint32_t getCapacity() const { return this->_capacity; }

  const std::vector<VkDescriptorImageInfo>& getImageInfos() {
    return this->_imageInfos;
  }

private:
  void _initImageInfo(const Texture& texture, uint32_t slotIdx);

  std::vector<VkDescriptorImageInfo> _imageInfos{};

  uint32_t _size;
  uint32_t _capacity;
};
} // namespace AltheaEngine