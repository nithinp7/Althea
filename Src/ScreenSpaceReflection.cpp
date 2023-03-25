#include "ScreenSpaceReflection.h"

#include "Application.h"

#include <cstdint>

namespace AltheaEngine {
namespace {
struct ConvolutionPushConstants {
  uint32_t width;
  uint32_t height;
  float roughness;
};
} // namespace

ScreenSpaceReflection::ScreenSpaceReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    // TODO: standardize global descriptor set
    VkDescriptorSetLayout globalSetLayout,
    const GBufferResources& gBuffer) {

  const VkExtent2D& extent = app.getSwapChainExtent();

  // TODO: Would it be useful to parameterize this?
  const uint32_t mipCount = 5;

  // Setup reflection buffer resources needed for SSR and convolution
  // Image
  ImageOptions imageOptions{};
  imageOptions.width = extent.width;
  imageOptions.height = extent.height;
  imageOptions.mipCount = mipCount;
  imageOptions.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  imageOptions.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                       VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
  this->_reflectionBuffer.image = Image(app, imageOptions);

  this->_reflectionBuffer.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  // Combined view
  ImageViewOptions viewOptions{};
  viewOptions.format = imageOptions.format;
  viewOptions.mipCount = mipCount;
  viewOptions.type = VK_IMAGE_VIEW_TYPE_2D;
  this->_reflectionBuffer.view =
      ImageView(app, this->_reflectionBuffer.image, viewOptions);

  // Individual mip views
  this->_mipViews.reserve(mipCount);
  for (uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex) {
    ImageViewOptions mipViewOptions{};
    mipViewOptions.format = imageOptions.format;
    mipViewOptions.mipCount = 1;
    mipViewOptions.mipBias = mipIndex;
    mipViewOptions.type = VK_IMAGE_VIEW_TYPE_2D;
    this->_mipViews.emplace_back(
        app,
        this->_reflectionBuffer.image,
        mipViewOptions);
  }

  // Sampler
  SamplerOptions samplerOptions{};
  samplerOptions.mipCount = mipCount;
  this->_reflectionBuffer.sampler = Sampler(app, samplerOptions);

  // Individual mip sampler
  SamplerOptions mipSamplerOptions{};
  this->_mipSampler = Sampler(app, mipSamplerOptions);

  // Setup material needed for reflection pass
  {
    DescriptorSetLayoutBuilder layoutBuilder{};
    GBufferResources::buildMaterial(layoutBuilder);
    this->_pGBufferMaterialAllocator =
        std::make_unique<DescriptorSetAllocator>(app, layoutBuilder, 1);
    this->_pGBufferMaterial =
        std::make_unique<Material>(app, *this->_pGBufferMaterialAllocator);
    gBuffer.bindTextures(this->_pGBufferMaterial->assign());
  }

  // Setup material needed for convolution pass
  {
    DescriptorSetLayoutBuilder layoutBuilder{};
    // Previous mip of reflection buffer.
    layoutBuilder.addTextureBinding(VK_SHADER_STAGE_COMPUTE_BIT)
        .addStorageImageBinding();
    this->_pConvolutionMaterialAllocator =
        std::make_unique<DescriptorSetAllocator>(app, layoutBuilder, mipCount);

    // Each convolution pass gets a material
    this->_convolutionMaterials.reserve(mipCount - 1);
    for (uint32_t mipIndex = 0; mipIndex < mipCount - 1; ++mipIndex) {
      Material& material = this->_convolutionMaterials.emplace_back(
          app,
          *this->_pConvolutionMaterialAllocator);
      material.assign()
          .bindTexture(this->_mipViews[mipIndex], this->_mipSampler)
          .bindStorageImage(this->_mipViews[mipIndex + 1], this->_mipSampler);
    }
  }

  // Setup reflection pass
  {
    std::vector<SubpassBuilder> subpassBuilders;
    {
      SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();
      subpassBuilder.colorAttachments.push_back(0);

      subpassBuilder
          .pipelineBuilder

          .addVertexShader(GEngineDirectory + "/Shaders/FullScreenQuad.vert")
          .addFragmentShader(GEngineDirectory + "/Shaders/SSR.frag")

          .setCullMode(VK_CULL_MODE_NONE) // ??

          .layoutBuilder.addDescriptorSet(globalSetLayout)
          .addDescriptorSet(this->_pGBufferMaterialAllocator->getLayout());
    }

    VkClearValue colorClear;
    colorClear.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
    std::vector<Attachment> attachments = {
        {ATTACHMENT_FLAG_COLOR,
         VK_FORMAT_R16G16B16A16_SFLOAT,
         colorClear,
         false,
         false}};

    this->_pReflectionPass = std::make_unique<RenderPass>(
        app,
        extent,
        std::move(attachments),
        std::move(subpassBuilders));

    std::vector<VkImageView> attachmentViews = {this->_mipViews[0]};
    this->_reflectionFrameBuffer =
        FrameBuffer(app, *this->_pReflectionPass, extent, attachmentViews);
  }

  // Setup convolution pass
  {
    ComputePipelineBuilder computeBuilder{};
    computeBuilder.setComputeShader(
        GEngineDirectory + "/Shaders/SSRGlossyConvolve.comp");
    computeBuilder.layoutBuilder
        .addDescriptorSet(this->_pConvolutionMaterialAllocator->getLayout())
        .addPushConstants<ConvolutionPushConstants>(
            VK_SHADER_STAGE_COMPUTE_BIT);

    this->_pConvolutionPass =
        std::make_unique<ComputePipeline>(app, std::move(computeBuilder));
  }
}

void ScreenSpaceReflection::transitionToAttachment(
    VkCommandBuffer commandBuffer) {
  // Only the first mip needs to be transitioned to be a color attachment.
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = this->_reflectionBuffer.image;
  barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);
}

void ScreenSpaceReflection::captureReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkDescriptorSet globalSet,
    const FrameContext& context) {
  // TODO make transition private function?
  this->transitionToAttachment(commandBuffer);

  ActiveRenderPass pass = this->_pReflectionPass->begin(
      app,
      commandBuffer,
      context,
      this->_reflectionFrameBuffer);
  pass.setGlobalDescriptorSets(gsl::span(&globalSet, 1));
  pass.getDrawContext().bindDescriptorSets(*this->_pGBufferMaterial);
  pass.getDrawContext().draw(3);
}

void ScreenSpaceReflection::convolveReflectionBuffer(
    const Application& app,
    VkCommandBuffer commandBuffer,
    const FrameContext& context) {

  this->_pConvolutionPass->bindPipeline(commandBuffer);

  const ImageOptions& imageDetails = this->_reflectionBuffer.image.getOptions();

  VkImageMemoryBarrier barriers[2];

  // Transition all mips except the first to allow for compute storage image
  // write
  VkImageMemoryBarrier writeBarrier{};
  writeBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  writeBarrier.image = this->_reflectionBuffer.image;
  writeBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  writeBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  writeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  writeBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  writeBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  writeBarrier.subresourceRange.baseMipLevel = 1;
  writeBarrier.subresourceRange.levelCount = imageDetails.mipCount - 1;
  writeBarrier.subresourceRange.baseArrayLayer = 0;
  writeBarrier.subresourceRange.layerCount = 1;
  writeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  writeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &writeBarrier);

  // Will re-use this read barrier to enable reading a mip as a texture
  VkImageMemoryBarrier readBarrier{};
  readBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  readBarrier.image = this->_reflectionBuffer.image;
  readBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  readBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  readBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  readBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  readBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  readBarrier.subresourceRange.baseMipLevel = 0;
  readBarrier.subresourceRange.levelCount = 1;
  readBarrier.subresourceRange.baseArrayLayer = 0;
  readBarrier.subresourceRange.layerCount = 1;
  readBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  readBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  uint32_t previousMipWidth = imageDetails.width;
  uint32_t previousMipHeight = imageDetails.height;
  for (uint32_t mipLevel = 1; mipLevel < imageDetails.mipCount; ++mipLevel) {
    uint32_t mipWidth = imageDetails.width >> mipLevel;
    if (mipWidth == 0) {
      mipWidth = 1;
    }

    uint32_t mipHeight = imageDetails.height >> mipLevel;
    if (mipHeight == 0) {
      mipHeight = 1;
    }

    // Execute barrier to prepare the previous mip for reading
    readBarrier.subresourceRange.baseMipLevel = mipLevel - 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStage,
        dstStage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &readBarrier);

    // Dispatch compute work
    VkDescriptorSet material =
        this->_convolutionMaterials[mipLevel - 1].getCurrentDescriptorSet(
            context);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        this->_pConvolutionPass->getLayout(),
        0,
        1,
        &material,
        0,
        nullptr);

    ConvolutionPushConstants constants{};
    constants.width = mipWidth;
    constants.height = mipHeight;
    constants.roughness = static_cast<float>(mipLevel) / 4.0f;
    vkCmdPushConstants(
        commandBuffer,
        this->_pConvolutionPass->getLayout(),
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(ConvolutionPushConstants),
        &constants);

    uint32_t groupCountX = (mipWidth - 1) / 16 + 1;
    uint32_t groupCountY = (mipHeight - 1) / 16 + 1;
    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

    previousMipWidth = mipWidth;
    previousMipHeight = mipHeight;

    // After the first mip, the rest of the mips are being transitioned from
    // a compute storage image to texture read
    srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    readBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    readBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  }

  // Transition the last mip for shader read as well
  readBarrier.subresourceRange.baseMipLevel = imageDetails.mipCount - 1;
  vkCmdPipelineBarrier(
      commandBuffer,
      srcStage,
      dstStage,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &readBarrier);
}

void ScreenSpaceReflection::bindTexture(ResourcesAssignment& assignment) const {
  assignment.bindTexture(this->_reflectionBuffer);
}

} // namespace AltheaEngine