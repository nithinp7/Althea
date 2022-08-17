#pragma once

#include "Application.h"

#include <vulkan/vulkan.h>

namespace CesiumGltf {
struct Model;
struct Texture;
} // namespace CesiumGltf

class Texture {
public: 
  Texture(
      const Application& app,
      const CesiumGltf::Model& model,
      const CesiumGltf::Texture& texture);
  Texture(Texture&& rhs);
  ~Texture();

  VkImageView getImageView() const {
    return this->_textureImageView;
  }

  VkSampler getSampler() const {
    return this->_textureSampler;
  }
private:
  VkDevice _device;

  VkImage _textureImage;
  VkDeviceMemory _textureImageMemory;

  VkImageView _textureImageView;
  VkSampler _textureSampler;

  bool _needsDestruction = true;
};

