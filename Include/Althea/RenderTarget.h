#pragma once

#include "Image.h"
#include "ImageResource.h"
#include "ImageView.h"
#include "Library.h"

#include <vulkan/vulkan.h>

namespace AltheaEngine {
class Application;

enum class ALTHEA_API RenderTargetType { SceneCapture2D, SceneCaptureCube };

class ALTHEA_API RenderTarget {
public:
  RenderTarget() = default;
  RenderTarget(
      const Application& app,
      VkCommandBuffer commandBuffer,
      const VkExtent2D& extent,
      RenderTargetType type);

  void transitionToAttachment(VkCommandBuffer commandBuffer);
  void transitionToTexture(VkCommandBuffer commandBuffer);

  Image& getColorImage() { return this->_color.image; }

  const Image& getColorImage() const { return this->_color.image; }

  ImageView& getColorView() { return this->_color.view; }

  const ImageView& getColorView() const { return this->_color.view; }

  Sampler& getColorSampler() { return this->_color.sampler; }

  const Sampler& getColorSampler() const { return this->_color.sampler; }

  Image& getDepthImage() { return this->_depthImage; }

  const Image& getDepthImage() const { return this->_depthImage; }

  ImageView& getDepthView() { return this->_depthImageView; }

  const ImageView& getDepthView() const { return this->_depthImageView; }

private:
  ImageResource _color{};
  Image _depthImage{};
  ImageView _depthImageView{};
};
}; // namespace AltheaEngine