#include "DebugDraw.h"

#include "Application.h"

#include <vector>

namespace AltheaEngine {
namespace {
struct DebugLinePushConstants {
  uint32_t bufferIdx;
  uint32_t primitiveOffset;
};
} // namespace

DebugDraw::DebugDraw(
    const Application& app,
    const GBufferResources& gBuffer,
    VkDescriptorSetLayout debugDescriptorSetLayout,
    uint32_t debugBufferBinding) {

  std::vector<SubpassBuilder> builders;

  // Draw debug line primitives
  {
    SubpassBuilder& subpassBuilder = builders.emplace_back();

    // GBuffer and depth attachments
    subpassBuilder.colorAttachments = {0, 1, 2, 3};
    subpassBuilder.depthAttachment = 4;

    ShaderDefines defs;
    defs.emplace("DEBUG_BUFFER_BINDING", std::to_string(debugBufferBinding));

    subpassBuilder.pipelineBuilder.setPrimitiveType(PrimitiveType::LINES)
        .addVertexShader(GEngineDirectory + "/Shaders/Debug/Lines.vert", defs)
        .addFragmentShader(GEngineDirectory + "/Shaders/Debug/Lines.frag", defs)
        .layoutBuilder.addDescriptorSet(debugDescriptorSetLayout)
        .addPushConstants<DebugLinePushConstants>(VK_SHADER_STAGE_ALL);
  }

  std::vector<Attachment> attachments = gBuffer.getAttachmentDescriptions();
  this->_renderPass = RenderPass(
      app,
      app.getSwapChainExtent(),
      std::move(attachments),
      std::move(builders));
}

// TODO: Add draw indirect version

void DebugDraw::draw(
    VkCommandBuffer commandBuffer,
    uint32_t bufferIdx,
    uint32_t primOffs,
    uint32_t primCount) {
  {
    // ActiveRenderPass pass = this->_renderPass.begin();
    // pass.getDrawContext().updatePushConstants<DebugLinePushConstants>(
    //     {bufferIdx, primOffs});
    // vkCmdDraw(commandBuffer, 2 * primCount, 1, 0, 0);
  }
}
} // namespace AltheaEngine