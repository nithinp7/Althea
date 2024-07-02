#include "Debug/DebugDraw.h"
#include "Debug/DebugDrawCommon.h"

namespace AltheaEngine {

DebugDrawLines::DebugDrawLines(Application& app, uint32_t maxLinesCapacity) {
  m_lines = DynamicVertexBuffer<DebugVert>(app, 2 * maxLinesCapacity, true);
}

// IGBufferSubpass impl
void DebugDrawLines::registerGBufferSubpass(
    GraphicsPipelineBuilder& builder) const {

  builder.setPrimitiveType(PrimitiveType::LINES)
      .setLineWidth(2.5f)
      .addVertexInputBinding<DebugVert>(VK_VERTEX_INPUT_RATE_VERTEX)
      .addVertexAttribute(
          VertexAttributeType::VEC3,
          offsetof(DebugVert, position))
      .addVertexAttribute(VertexAttributeType::UINT, offsetof(DebugVert, color))

      .addVertexShader(GEngineDirectory + "/Shaders/Debug/Lines.vert")
      .addFragmentShader(GEngineDirectory + "/Shaders/Debug/Lines.frag")

      .layoutBuilder //
      .addPushConstants<DebugDrawPush>(VK_SHADER_STAGE_ALL);
}

void DebugDrawLines::beginGBufferSubpass(
    const DrawContext& context,
    BufferHandle globalResources,
    UniformHandle globalUniforms) {
  const FrameContext& frame = context.getFrame();

  m_lines.upload(frame.frameRingBufferIndex);
  {
    DebugDrawPush constants{};
    constants.globalResources = globalResources.index;
    constants.globalUniforms = globalUniforms.index;

    context.updatePushConstants(constants, 0);
    context.bindDescriptorSets();

    VkBuffer vb = m_lines.getBuffer();
    size_t offset = m_lines.getCurrentBufferOffset(frame.frameRingBufferIndex);
    vkCmdBindVertexBuffers(context.getCommandBuffer(), 0, 1, &vb, &offset);
    context.draw(2 * m_lineCount);
  }
}
} // namespace AltheaEngine