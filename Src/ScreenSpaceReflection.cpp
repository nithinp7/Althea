#include "ScreenSpaceReflection.h"

#include "Application.h"

#include <cstdint>

namespace AltheaEngine {
struct SSRPushConstants {
  uint32_t globalUniformsHandle;
  uint32_t globalResourcesHandle;
};

ScreenSpaceReflection::ScreenSpaceReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkDescriptorSetLayout globalSetLayout)
    : _reflectionBuffer(app, commandBuffer) {
  // Setup reflection pass
  {
    std::vector<SubpassBuilder> subpassBuilders;
    {
      SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();
      subpassBuilder.colorAttachments.push_back(0);

      subpassBuilder.pipelineBuilder
          .setDepthTesting(false)

          .addVertexShader(GEngineDirectory + "/Shaders/SSR.vert")
          .addFragmentShader(GEngineDirectory + "/Shaders/SSR.frag")

          .setCullMode(VK_CULL_MODE_NONE) // ??

          .layoutBuilder.addDescriptorSet(globalSetLayout)
          .addPushConstants<SSRPushConstants>(VK_SHADER_STAGE_ALL);
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
    this->_reflectionPass = RenderPass(
        app,
        extent,
        std::move(attachments),
        std::move(subpassBuilders));

    std::vector<VkImageView> attachmentViews = {
        this->_reflectionBuffer.getReflectionBufferTargetView()};
    this->_reflectionFrameBuffer =
        FrameBuffer(app, this->_reflectionPass, extent, attachmentViews);
  }
}

void ScreenSpaceReflection::captureReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkDescriptorSet globalSet,
    const FrameContext& context,
    UniformHandle globalUniforms,
    BufferHandle globalResources) {
  this->_reflectionBuffer.transitionToAttachment(commandBuffer);

  ActiveRenderPass pass = this->_reflectionPass.begin(
      app,
      commandBuffer,
      context,
      this->_reflectionFrameBuffer);
  pass.setGlobalDescriptorSets(gsl::span(&globalSet, 1));
  pass.getDrawContext().bindDescriptorSets();
  
  SSRPushConstants push{};
  push.globalUniformsHandle = globalUniforms.index;
  push.globalResourcesHandle = globalResources.index;

  pass.getDrawContext().updatePushConstants(push, 0);
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