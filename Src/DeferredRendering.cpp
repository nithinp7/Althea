#include "DeferredRendering.h"

#include "Application.h"
#include "Image.h"
#include "ImageView.h"
#include "Sampler.h"

namespace AltheaEngine {
/*static*/
void GBufferResources::buildMaterial(DescriptorSetLayoutBuilder& builder) {
  builder
      // GBuffer Position
      .addTextureBinding(VK_SHADER_STAGE_FRAGMENT_BIT)
      // GBuffer Normal
      .addTextureBinding(VK_SHADER_STAGE_FRAGMENT_BIT)
      // GBuffer Albedo
      .addTextureBinding(VK_SHADER_STAGE_FRAGMENT_BIT)
      // GBuffer Metallic-Roughness-Occlusion
      .addTextureBinding(VK_SHADER_STAGE_FRAGMENT_BIT);
}

GBufferResources::GBufferResources(const Application& app) {
  const VkExtent2D& extent = app.getSwapChainExtent();

  // Create image resources for the GBuffer
  ImageOptions imageOptions{};
  imageOptions.width = extent.width;
  imageOptions.height = extent.height;
  imageOptions.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  imageOptions.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageOptions.usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  SamplerOptions samplerOptions{};

  ImageViewOptions viewOptions{};
  viewOptions.type = VK_IMAGE_VIEW_TYPE_2D;
  viewOptions.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  viewOptions.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

  this->_position.image = Image(app, imageOptions);
  this->_position.sampler = Sampler(app, samplerOptions);
  this->_position.view = ImageView(app, this->_position.image, viewOptions);

  this->_normal.image = Image(app, imageOptions);
  this->_normal.sampler = Sampler(app, samplerOptions);
  this->_normal.view = ImageView(app, this->_normal.image, viewOptions);

  imageOptions.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewOptions.format = VK_FORMAT_R8G8B8A8_UNORM;
  this->_albedo.image = Image(app, imageOptions);
  this->_albedo.sampler = Sampler(app, samplerOptions);
  this->_albedo.view = ImageView(app, this->_albedo.image, viewOptions);

  this->_metallicRoughnessOcclusion.image = Image(app, imageOptions);
  this->_metallicRoughnessOcclusion.sampler = Sampler(app, samplerOptions);
  this->_metallicRoughnessOcclusion.view =
      ImageView(app, this->_metallicRoughnessOcclusion.image, viewOptions);

  VkClearValue posClear;
  posClear.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  VkClearValue colorClear;
  colorClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  VkClearValue depthClear;
  depthClear.depthStencil = {1.0f, 0};

  this->_attachmentDescriptions = {// GBuffer Position
                                   Attachment{
                                       ATTACHMENT_FLAG_COLOR,
                                       VK_FORMAT_R16G16B16A16_SFLOAT,
                                       posClear,
                                       false,
                                       false,
                                       true},

                                   // GBuffer Normal
                                   Attachment{
                                       ATTACHMENT_FLAG_COLOR,
                                       VK_FORMAT_R16G16B16A16_SFLOAT,
                                       colorClear,
                                       false,
                                       false,
                                       true},

                                   // GBuffer Albedo
                                   Attachment{
                                       ATTACHMENT_FLAG_COLOR,
                                       VK_FORMAT_R8G8B8A8_UNORM,
                                       colorClear,
                                       false,
                                       false,
                                       true},

                                   // GBuffer Metallic-Roughness-Occlusion
                                   Attachment{
                                       ATTACHMENT_FLAG_COLOR,
                                       VK_FORMAT_R8G8B8A8_UNORM,
                                       colorClear,
                                       false,
                                       false,
                                       true},

                                   // Depth buffer
                                   Attachment{
                                       ATTACHMENT_FLAG_DEPTH,
                                       app.getDepthImageFormat(),
                                       depthClear,
                                       false,
                                       false,
                                       true}};

  this->_attachmentViews = {
      this->_position.view,
      this->_normal.view,
      this->_albedo.view,
      this->_metallicRoughnessOcclusion.view,
      app.getDepthImageView()};
}

void GBufferResources::bindTextures(ResourcesAssignment& assignment) const {
  assignment.bindTexture(this->_position)
      .bindTexture(this->_normal)
      .bindTexture(this->_albedo)
      .bindTexture(this->_metallicRoughnessOcclusion);
}

void GBufferResources::transitionToAttachment(VkCommandBuffer commandBuffer) {
  // Transition GBuffer to attachment
  this->_position.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  this->_normal.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  this->_albedo.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  this->_metallicRoughnessOcclusion.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void GBufferResources::transitionToTextures(VkCommandBuffer commandBuffer) {
  this->_position.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  this->_normal.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  this->_albedo.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  this->_metallicRoughnessOcclusion.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}
} // namespace AltheaEngine