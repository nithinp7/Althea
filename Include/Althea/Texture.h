#pragma once

#include "Library.h"

#include "Allocator.h"
#include "Application.h"
#include "ImageView.h"
#include "Sampler.h"

#include <vulkan/vulkan.h>

namespace CesiumGltf {
struct ALTHEA_API Model;
struct ALTHEA_API Texture;
struct ALTHEA_API ImageCesium;
struct ALTHEA_API Sampler;
} // namespace CesiumGltf

namespace AltheaEngine {
class ALTHEA_API Texture {
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

  VkImage getImage() const { return this->_allocation.getImage(); }

  VkImageView getImageView() const { return this->_imageView; }

  VkSampler getSampler() const { return this->_sampler; }

private:
  void _initTexture(
      const Application& app,
      const CesiumGltf::ImageCesium& image,
      const CesiumGltf::Sampler& sampler,
      bool srgb);

  ImageAllocation _allocation;

  ImageView _imageView;
  Sampler _sampler;
};
} // namespace AltheaEngine
