#include "RenderTarget.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
RenderTargetCollection::RenderTargetCollection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    const VkExtent2D& extent,
    RenderTargetType type,
    uint32_t targetCount)
    : _targetCount(targetCount) {
  ImageOptions imageOptions{};
  imageOptions.width = extent.width;
  imageOptions.height = extent.height;

  imageOptions.mipCount = 1;

  if (type == RenderTargetType::SceneCapture2D) {
    imageOptions.layerCount = targetCount;
  } else if (type == RenderTargetType::SceneCaptureCube) {
    if (extent.width != extent.height) {
      throw std::runtime_error("Attempting to create a RenderTarget cubemap "
                               "with mismatched width and height.");
    }

    imageOptions.layerCount = 6 * targetCount;
  }

  // TODO: Verify this works as expected
  imageOptions.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  imageOptions.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageOptions.usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  if (type == RenderTargetType::SceneCaptureCube) {
    imageOptions.createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  }

  this->_colorImage = Image(app, imageOptions);

  SamplerOptions samplerOptions{};
  samplerOptions.mipCount = imageOptions.mipCount;
  samplerOptions.magFilter = VK_FILTER_LINEAR;
  samplerOptions.minFilter = VK_FILTER_LINEAR;
  samplerOptions.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  this->_colorImageSampler = Sampler(app, samplerOptions);

  // Each image view will correspond to a unique render target within
  // the same overall image.
  uint32_t viewLayersPerTarget =
      type == RenderTargetType::SceneCaptureCube ? 6 : 1;
  ImageViewOptions colorTargetViewOptions{};
  colorTargetViewOptions.format = imageOptions.format;
  colorTargetViewOptions.mipCount = imageOptions.mipCount;
  colorTargetViewOptions.layerCount = viewLayersPerTarget;
  colorTargetViewOptions.type = type == RenderTargetType::SceneCaptureCube
                                    ? VK_IMAGE_VIEW_TYPE_CUBE
                                    : VK_IMAGE_VIEW_TYPE_2D,
  colorTargetViewOptions.aspectFlags = imageOptions.aspectMask;
  this->_colorTargetImageViews.reserve(targetCount);
  for (uint32_t i = 0; i < targetCount; ++i) {
    colorTargetViewOptions.baseLayer = i * viewLayersPerTarget;
    this->_colorTargetImageViews.emplace_back(
        app,
        this->_colorImage,
        colorTargetViewOptions);
  }

  // We also create one texture array image view for shader sampling.
  ImageViewOptions colorTextureArrayViewOptions{};
  colorTextureArrayViewOptions.format = imageOptions.format;
  colorTextureArrayViewOptions.mipCount = imageOptions.mipCount;
  colorTextureArrayViewOptions.layerCount = targetCount * viewLayersPerTarget;
  colorTextureArrayViewOptions.type = type == RenderTargetType::SceneCaptureCube
                                          ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
                                          : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
  colorTextureArrayViewOptions.aspectFlags = imageOptions.aspectMask;
  this->_colorTextureArrayView =
      ImageView(app, this->_colorImage, colorTextureArrayViewOptions);

  ImageOptions depthImageOptions{};
  depthImageOptions.width = imageOptions.width;
  depthImageOptions.height = imageOptions.height;
  // TODO: change mipcount? HZB?
  depthImageOptions.mipCount = 1;
  depthImageOptions.layerCount = imageOptions.layerCount;
  depthImageOptions.format = app.getDepthImageFormat();
  // TODO: stencil bit?
  depthImageOptions.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depthImageOptions.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  if (type == RenderTargetType::SceneCaptureCube) {
    depthImageOptions.createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  }

  this->_depthImage = Image(app, depthImageOptions);

  // Each depth view will correspond to a unique render target within
  // the same overall image.
  ImageViewOptions depthViewOptions{};
  depthViewOptions.format = app.getDepthImageFormat();
  depthViewOptions.layerCount = viewLayersPerTarget;
  depthViewOptions.aspectFlags = depthImageOptions.aspectMask;
  depthViewOptions.type = type == RenderTargetType::SceneCaptureCube
                              ? VK_IMAGE_VIEW_TYPE_CUBE
                              : VK_IMAGE_VIEW_TYPE_2D;
  this->_depthTargetImageViews.reserve(targetCount);
  for (uint32_t i = 0; i < targetCount; ++i) {
    this->_depthTargetImageViews.emplace_back(
        app,
        this->_depthImage,
        depthViewOptions);
  }

  this->_depthImage.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
}

void RenderTargetCollection::transitionToAttachment(
    VkCommandBuffer commandBuffer) {
  this->_colorImage.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void RenderTargetCollection::transitionToTexture(
    VkCommandBuffer commandBuffer) {
  this->_colorImage.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}
} // namespace AltheaEngine