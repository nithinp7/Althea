#pragma once

#include "Application.h"

#include <vulkan/vulkan.h>

namespace CesiumGltf {
struct Model;
struct Texture;
struct ImageCesium;
struct Sampler;
} // namespace CesiumGltf

namespace AltheaEngine {
class Texture {
public:
  Texture(
      const Application& app,
      const CesiumGltf::Model& model,
      const CesiumGltf::Texture& texture,
      bool srgb);
  Texture(
      const Application& app,
      const CesiumGltf::ImageCesium& image,
      const CesiumGltf::Sampler& sampler,
      bool srgb);
  Texture(const Texture& rhs) = delete;
  Texture& operator=(const Texture& rhs) = delete;
  Texture(Texture&& rhs) = delete;
  ~Texture();

  VkImageView getImageView() const { return this->_textureImageView; }

  VkSampler getSampler() const { return this->_textureSampler; }

private:
  void _initTexture(
      const Application& app,
      const CesiumGltf::ImageCesium& image,
      const CesiumGltf::Sampler& sampler,
      bool srgb);

  VkDevice _device;

  VkImage _textureImage;
  VkDeviceMemory _textureImageMemory;

  VkImageView _textureImageView;
  VkSampler _textureSampler;
};
} // namespace AltheaEngine
