#pragma once

#include "Image.h"
#include "ImageView.h"
#include "Library.h"
#include "Sampler.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {
class Application;

enum ALTHEA_API RenderTargetFlags {
  SceneCapture2D = 1,
  SceneCaptureCube = 2,
  EnableColorTarget = 4,
  EnableDepthTarget = 16
};

class ALTHEA_API RenderTargetCollection {
public:
  RenderTargetCollection() = default;
  RenderTargetCollection(
      const Application& app,
      VkCommandBuffer commandBuffer,
      const VkExtent2D& extent,
      uint32_t targetCount,
      int renderTargetFlags = RenderTargetFlags::SceneCapture2D |
                              RenderTargetFlags::EnableColorTarget);

  void transitionToAttachment(VkCommandBuffer commandBuffer);
  void transitionToTexture(VkCommandBuffer commandBuffer);

  uint32_t getTargetCount() const { return this->_targetCount; }

  Image& getColorImage() { return this->_colorImage; }

  const Image& getColorImage() const { return this->_colorImage; }

  ImageView& getColorTextureArrayView() { return this->_colorTextureArrayView; }

  const ImageView& getColorTextureArrayView() const {
    return this->_colorTextureArrayView;
  }

  Sampler& getColorSampler() { return this->_colorImageSampler; }

  const Sampler& getColorSampler() const { return this->_colorImageSampler; }

  ImageView& getTargetColorView(uint32_t targetIndex) {
    return this->_colorTargetImageViews[targetIndex];
  }

  const ImageView& getTargetColorView(uint32_t targetIndex) const {
    return this->_colorTargetImageViews[targetIndex];
  }

  Image& getDepthImage() { return this->_depthImage; }

  const Image& getDepthImage() const { return this->_depthImage; }

  ImageView& getDepthTextureArrayView() { return this->_depthTextureArrayView; }

  const ImageView& getDepthTextureArrayView() const {
    return this->_depthTextureArrayView;
  }

  Sampler& getDepthSampler() { return this->_depthImageSampler; }

  const Sampler& getDepthSampler() const { return this->_depthImageSampler; }

  ImageView& getTargetDepthView(uint32_t targetIndex) {
    return this->_depthTargetImageViews[targetIndex];
  }

  const ImageView& getTargetDepthView(uint32_t targetIndex) const {
    return this->_depthTargetImageViews[targetIndex];
  }

private:
  uint32_t _targetCount;
  int _flags;

  Image _colorImage{};
  Sampler _colorImageSampler{};
  ImageView _colorTextureArrayView{};
  std::vector<ImageView> _colorTargetImageViews{};

  Image _depthImage{};
  Sampler _depthImageSampler{};
  ImageView _depthTextureArrayView{};
  std::vector<ImageView> _depthTargetImageViews{};
};
}; // namespace AltheaEngine