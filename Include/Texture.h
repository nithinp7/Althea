#pragma once

#include "Allocator.h"
#include "Application.h"
#include "ImageView.h"
#include "Sampler.h"

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

  // Move-only semantics
  Texture(Texture&& rhs) = default;
  Texture& operator=(Texture&& rhs) = default;

  Texture(const Texture& rhs) = delete;
  Texture& operator=(const Texture& rhs) = delete;

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
