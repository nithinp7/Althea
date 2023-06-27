#include "RenderTarget.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
RenderTargetCollection::RenderTargetCollection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    const VkExtent2D& extent,
    uint32_t targetCount,
    int renderTargetFlags)
    : _extent(extent), _targetCount(targetCount), _flags(renderTargetFlags) {
  ImageOptions imageOptions{};
  imageOptions.width = extent.width;
  imageOptions.height = extent.height;

  imageOptions.mipCount = 1;

  if (renderTargetFlags & RenderTargetFlags::SceneCapture2D) {
    imageOptions.layerCount = targetCount;
  } else if (renderTargetFlags & RenderTargetFlags::SceneCaptureCube) {
    if (extent.width != extent.height) {
      throw std::runtime_error("Attempting to create a RenderTarget cubemap "
                               "with mismatched width and height.");
    }

    imageOptions.layerCount = 6 * targetCount;
  }

  // Each image view will correspond to a unique render target within
  // the same overall image.
  uint32_t viewLayersPerTarget =
      (renderTargetFlags & RenderTargetFlags::SceneCaptureCube) ? 6 : 1;

  if (renderTargetFlags & RenderTargetFlags::EnableColorTarget) {
    // TODO: Expose option to change color format
    imageOptions.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageOptions.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageOptions.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    if (renderTargetFlags & RenderTargetFlags::SceneCaptureCube) {
      imageOptions.createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    this->_colorImage = Image(app, imageOptions);

    SamplerOptions samplerOptions{};
    samplerOptions.mipCount = imageOptions.mipCount;

    this->_colorImageSampler = Sampler(app, samplerOptions);

    ImageViewOptions colorTargetViewOptions{};
    colorTargetViewOptions.format = imageOptions.format;
    colorTargetViewOptions.mipCount = imageOptions.mipCount;
    colorTargetViewOptions.layerCount = viewLayersPerTarget;
    colorTargetViewOptions.type =
        (renderTargetFlags & RenderTargetFlags::SceneCaptureCube)
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
    colorTextureArrayViewOptions.type =
        (renderTargetFlags & RenderTargetFlags::SceneCaptureCube)
            ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
            : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    colorTextureArrayViewOptions.aspectFlags = imageOptions.aspectMask;
    this->_colorTextureArrayView =
        ImageView(app, this->_colorImage, colorTextureArrayViewOptions);
  }

  // Need depth image regardless of if we need a depth render target
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

  if (renderTargetFlags & RenderTargetFlags::EnableDepthTarget) {
    depthImageOptions.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }

  if (renderTargetFlags & RenderTargetFlags::SceneCaptureCube) {
    depthImageOptions.createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  }

  this->_depthImage = Image(app, depthImageOptions);

  // Each depth view will correspond to a unique render target within
  // the same overall image.
  ImageViewOptions depthViewOptions{};
  depthViewOptions.format = app.getDepthImageFormat();
  depthViewOptions.layerCount = viewLayersPerTarget;
  depthViewOptions.aspectFlags = depthImageOptions.aspectMask;
  depthViewOptions.type =
      (renderTargetFlags & RenderTargetFlags::SceneCaptureCube)
          ? VK_IMAGE_VIEW_TYPE_CUBE
          : VK_IMAGE_VIEW_TYPE_2D;
  this->_depthTargetImageViews.reserve(targetCount);
  for (uint32_t i = 0; i < targetCount; ++i) {
    depthViewOptions.baseLayer = i * viewLayersPerTarget;
    this->_depthTargetImageViews.emplace_back(
        app,
        this->_depthImage,
        depthViewOptions);
  }

  // If we are using the depth buffer as a texture, then we need a texture array
  // view and sampler
  if (renderTargetFlags & RenderTargetFlags::EnableDepthTarget) {

    SamplerOptions samplerOptions{};
    samplerOptions.mipCount = depthImageOptions.mipCount;

    this->_depthImageSampler = Sampler(app, samplerOptions);

    // We also create one texture array image view for shader sampling.
    ImageViewOptions depthTextureArrayViewOptions{};
    depthTextureArrayViewOptions.format = depthImageOptions.format;
    depthTextureArrayViewOptions.mipCount = depthImageOptions.mipCount;
    depthTextureArrayViewOptions.layerCount = targetCount * viewLayersPerTarget;
    depthTextureArrayViewOptions.type =
        (renderTargetFlags & RenderTargetFlags::SceneCaptureCube)
            ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
            : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    depthTextureArrayViewOptions.aspectFlags = depthImageOptions.aspectMask;
    this->_depthTextureArrayView =
        ImageView(app, this->_depthImage, depthTextureArrayViewOptions);
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
  if (this->_flags & RenderTargetFlags::EnableColorTarget) {
    this->_colorImage.transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  }

  if (this->_flags & RenderTargetFlags::EnableDepthTarget) {
    // If we are using depth as a render target, we may have transitioned
    // the resource for reading, so transition it back for use during drawing
    // here.
    this->_depthImage.transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
  }
}

void RenderTargetCollection::transitionToTexture(
    VkCommandBuffer commandBuffer) {
  if (this->_flags & RenderTargetFlags::EnableColorTarget) {
    this->_colorImage.transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  }

  if (this->_flags & RenderTargetFlags::EnableDepthTarget) {
    this->_depthImage.transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  }
}
} // namespace AltheaEngine