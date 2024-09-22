#pragma once

#include "BindlessHandle.h"
#include "Library.h"
#include "ImageResource.h"

#include <vulkan/vulkan.h>

namespace CesiumGltf {
struct Model;
struct Texture;
struct ImageCesium;
struct Sampler;
} // namespace CesiumGltf

namespace AltheaEngine {
class Application;
class SingleTimeCommandBuffer;
class GlobalHeap;

class ALTHEA_API Texture {
public:
  Texture(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const CesiumGltf::Model& model,
      const CesiumGltf::Texture& texture,
      bool srgb);
  Texture(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const CesiumGltf::ImageCesium& image,
      const CesiumGltf::Sampler& sampler,
      bool srgb);

  VkImageView getImageView() const { return m_resource.view; }
  VkSampler getSampler() const { return m_resource.sampler; }
  void registerToHeap(GlobalHeap& heap);
  TextureHandle getHandle() const { return m_resource.textureHandle; }
private:
  void _initTexture(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const CesiumGltf::ImageCesium& image,
      const CesiumGltf::Sampler& sampler,
      bool srgb);

  ImageResource m_resource;
};
} // namespace AltheaEngine
