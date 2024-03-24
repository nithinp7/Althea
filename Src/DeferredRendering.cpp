#include "DeferredRendering.h"

#include "Application.h"
#include "GlobalHeap.h"
#include "Image.h"
#include "ImageView.h"
#include "RenderPass.h"
#include "Sampler.h"

namespace AltheaEngine {
/*static*/
void GBufferResources::setupAttachments(SubpassBuilder& builder) {
  // The GBuffer contains the following color attachments
  // 0. Normal
  // 1. Albedo
  // 2. Metallic-Roughness-Occlusion
  // 3. Depth
  builder.colorAttachments = {0, 1, 2};
  builder.depthAttachment = 3;
}

/*static*/
void GBufferResources::buildMaterial(
    DescriptorSetLayoutBuilder& builder,
    VkShaderStageFlags shaderStages) {
  builder
      // GBuffer Position
      .addTextureBinding(shaderStages)
      // GBuffer Normal
      .addTextureBinding(shaderStages)
      // GBuffer Albedo
      .addTextureBinding(shaderStages)
      // GBuffer Metallic-Roughness-Occlusion
      .addTextureBinding(shaderStages);
}

GBufferResources::GBufferResources(const Application& app) {
  const VkExtent2D& extent = app.getSwapChainExtent();

  // Create image resources for the GBuffer

  ImageOptions depthOptions{};
  depthOptions.width = extent.width;
  depthOptions.height = extent.height;
  // TODO: change mipcount? HZB?
  depthOptions.mipCount = 1;
  depthOptions.layerCount = 1;
  depthOptions.format = app.getDepthImageFormat();
  // TODO: stencil bit?
  depthOptions.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depthOptions.usage =
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  this->_depthA.image = Image(app, depthOptions);
  this->_depthB.image = Image(app, depthOptions);

  ImageViewOptions depthViewOptions{};
  depthViewOptions.format = app.getDepthImageFormat();
  depthViewOptions.aspectFlags = depthOptions.aspectMask;
  this->_depthA.view = ImageView(app, this->_depthA.image, depthViewOptions);
  this->_depthB.view = ImageView(app, this->_depthB.image, depthViewOptions);

  SamplerOptions depthSampler{};
  this->_depthA.sampler = Sampler(app, depthSampler);
  this->_depthB.sampler = Sampler(app, depthSampler);

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
  colorClear.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  VkClearValue depthClear;
  depthClear.depthStencil = {1.0f, 0};

  this->_attachmentDescriptions = {// GBuffer Normal
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

  this->_attachmentViewsA = {
      this->_normal.view,
      this->_albedo.view,
      this->_metallicRoughnessOcclusion.view,
      this->_depthA.view};
  this->_attachmentViewsB = {
      this->_normal.view,
      this->_albedo.view,
      this->_metallicRoughnessOcclusion.view,
      this->_depthB.view};
}

void GBufferResources::bindTextures(ResourcesAssignment& assignment) const {
  assignment.bindTexture(this->_depthA)
      .bindTexture(this->_normal)
      .bindTexture(this->_albedo)
      .bindTexture(this->_metallicRoughnessOcclusion);
}

void GBufferResources::transitionToAttachment(VkCommandBuffer commandBuffer) {
  // Transition GBuffer to attachment
  this->_depthA.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
  this->_depthB.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
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
  this->_depthA.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  this->_depthB.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  this->_normal.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  this->_albedo.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  this->_metallicRoughnessOcclusion.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void GBufferResources::registerToHeap(GlobalHeap& heap) {
  this->_depthAHandle = heap.registerTexture();
  this->_depthBHandle = heap.registerTexture();
  this->_normalHandle = heap.registerTexture();
  this->_albedoHandle = heap.registerTexture();
  this->_metallicRoughnessOcclusionHandle = heap.registerTexture();

  heap.updateTexture(
      this->_depthAHandle,
      this->_depthA.view,
      this->_depthA.sampler);
  heap.updateTexture(
      this->_depthBHandle,
      this->_depthB.view,
      this->_depthB.sampler);
  heap.updateTexture(
      this->_normalHandle,
      this->_normal.view,
      this->_normal.sampler);
  heap.updateTexture(
      this->_albedoHandle,
      this->_albedo.view,
      this->_albedo.sampler);
  heap.updateTexture(
      this->_metallicRoughnessOcclusionHandle,
      this->_metallicRoughnessOcclusion.view,
      this->_metallicRoughnessOcclusion.sampler);
}
} // namespace AltheaEngine