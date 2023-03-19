#include "RenderTarget.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
RenderTarget::RenderTarget(
    const Application& app,
    VkCommandBuffer commandBuffer,
    const VkExtent2D& extent,
    RenderTargetType type) {
  ImageOptions imageOptions{};
  imageOptions.width = extent.width;
  imageOptions.height = extent.height;

  imageOptions.mipCount = 1;

  if (type == RenderTargetType::SceneCapture2D) {
    imageOptions.layerCount = 1;
  } else if (type == RenderTargetType::SceneCaptureCube) {
    if (extent.width != extent.height) {
      throw std::runtime_error("Attempting to create a RenderTarget cubemap "
                               "with mismatched width and height.");
    }

    imageOptions.layerCount = 6;
  }

  // TODO: Verify this works as expected
  imageOptions.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  imageOptions.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageOptions.usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  if (type == RenderTargetType::SceneCaptureCube) {
    imageOptions.createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  }

  this->_color.image = Image(app, imageOptions);

  SamplerOptions samplerOptions{};
  samplerOptions.mipCount = imageOptions.mipCount;
  samplerOptions.magFilter = VK_FILTER_LINEAR;
  samplerOptions.minFilter = VK_FILTER_LINEAR;
  samplerOptions.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  this->_color.sampler = Sampler(app, samplerOptions);

  this->_color.view = ImageView(
      app,
      this->_color.image.getImage(),
      imageOptions.format,
      imageOptions.mipCount,
      imageOptions.layerCount,
      type == RenderTargetType::SceneCaptureCube ? VK_IMAGE_VIEW_TYPE_CUBE
                                                 : VK_IMAGE_VIEW_TYPE_2D,
      imageOptions.aspectMask);

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
  this->_depthImageView = ImageView(
      app,
      this->_depthImage.getImage(),
      app.getDepthImageFormat(),
      1,
      depthImageOptions.layerCount,
      type == RenderTargetType::SceneCaptureCube ? VK_IMAGE_VIEW_TYPE_CUBE
                                                 : VK_IMAGE_VIEW_TYPE_2D,
      depthImageOptions.aspectMask);

  this->_depthImage.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
}

void RenderTarget::transitionToAttachment(VkCommandBuffer commandBuffer) {
  this->_color.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void RenderTarget::transitionToTexture(VkCommandBuffer commandBuffer) {
  this->_color.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}
} // namespace AltheaEngine