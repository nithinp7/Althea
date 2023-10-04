#include "ScreenSpaceReflection.h"

#include "Application.h"

#include <cstdint>

namespace AltheaEngine {

ScreenSpaceReflection::ScreenSpaceReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    // TODO: standardize global descriptor set
    VkDescriptorSetLayout globalSetLayout,
    const GBufferResources& gBuffer)
    : _reflectionBuffer(app, commandBuffer) {

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

  // Setup reflection pass
  {
    std::vector<SubpassBuilder> subpassBuilders;
    {
      SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();
      subpassBuilder.colorAttachments.push_back(0);

      subpassBuilder.pipelineBuilder
          .setDepthTesting(false)

          .addVertexShader(
              GEngineDirectory + "/Shaders/Misc/FullScreenQuad.vert")
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

    const VkExtent2D& extent = app.getSwapChainExtent();
    this->_pReflectionPass = std::make_unique<RenderPass>(
        app,
        extent,
        std::move(attachments),
        std::move(subpassBuilders));

    std::vector<VkImageView> attachmentViews = {
        this->_reflectionBuffer.getReflectionBufferTargetView()};
    this->_reflectionFrameBuffer =
        FrameBuffer(app, *this->_pReflectionPass, extent, attachmentViews);
  }
}

void ScreenSpaceReflection::captureReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkDescriptorSet globalSet,
    const FrameContext& context) {
  this->_reflectionBuffer.transitionToAttachment(commandBuffer);

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
  this->_reflectionBuffer.convolveReflectionBuffer(
      app,
      commandBuffer,
      context,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void ScreenSpaceReflection::bindTexture(ResourcesAssignment& assignment) const {
  this->_reflectionBuffer.bindTexture(assignment);
}

} // namespace AltheaEngine