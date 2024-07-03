#include "Debug/DebugDraw.h"

#include "Debug/DebugDrawCommon.h"
#include "ShapeUtilities.h"

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

DebugDrawCapsules::DebugDrawCapsules(
    Application& app,
    VkCommandBuffer commandBuffer,
    uint32_t capacity) {
  m_capsules = DynamicVertexBuffer<CapsuleInst>(app, capacity, true);
  ShapeUtilities::createSphere(
      app,
      commandBuffer,
      m_sphere.verts,
      m_sphere.indices,
      8,
      1.0f,
      true);
  ShapeUtilities::createCylinder(
      app,
      commandBuffer,
      m_cylinder.verts,
      m_cylinder.indices,
      10,
      true);
}

// IGBufferSubpass impl
void DebugDrawCapsules::registerGBufferSubpass(
    GraphicsPipelineBuilder& builder) const {

  builder.setPrimitiveType(PrimitiveType::LINES)
      .setLineWidth(2.5f)
      .addVertexInputBinding<CapsuleInst>(VK_VERTEX_INPUT_RATE_INSTANCE)
      .addVertexAttribute(VertexAttributeType::VEC3, offsetof(CapsuleInst, a))
      .addVertexAttribute(VertexAttributeType::VEC3, offsetof(CapsuleInst, b))
      .addVertexAttribute(
          VertexAttributeType::FLOAT,
          offsetof(CapsuleInst, radius))
      .addVertexAttribute(
          VertexAttributeType::UINT,
          offsetof(CapsuleInst, color))
      .addVertexInputBinding<glm::vec3>(VK_VERTEX_INPUT_RATE_VERTEX)
      .addVertexAttribute(VertexAttributeType::VEC3, 0)

      .addVertexShader(GEngineDirectory + "/Shaders/Debug/Capsule.vert")
      .addFragmentShader(GEngineDirectory + "/Shaders/Debug/Capsule.frag")

      .layoutBuilder //
      .addPushConstants<DebugDrawPush>(VK_SHADER_STAGE_ALL);
}

void DebugDrawCapsules::beginGBufferSubpass(
    const DrawContext& context,
    BufferHandle globalResources,
    UniformHandle globalUniforms) {
  const FrameContext& frame = context.getFrame();

  m_capsules.upload(frame.frameRingBufferIndex);
  {
    DebugDrawPush constants{};
    constants.globalResources = globalResources.index;
    constants.globalUniforms = globalUniforms.index;

    context.bindDescriptorSets();

    VkBuffer vbs[2] = {
        m_capsules.getBuffer(),
        m_sphere.verts.getAllocation().getBuffer()};
    size_t offsets[2] = {
        m_capsules.getCurrentBufferOffset(frame.frameRingBufferIndex),
        0};
    vkCmdBindVertexBuffers(context.getCommandBuffer(), 0, 2, vbs, offsets);
    context.bindIndexBuffer(m_sphere.indices);

    // phase sphere A
    constants.extras0 = 0;
    context.updatePushConstants(constants, 0);

    context.drawIndexed(m_sphere.indices.getIndexCount(), m_count);

    // phase sphere B
    constants.extras0 = 1;
    context.updatePushConstants(constants, 0);
    
    context.drawIndexed(m_sphere.indices.getIndexCount(), m_count);

    // phase cylinder
    constants.extras0 = 2;
    context.updatePushConstants(constants, 0);

    vbs[1] = m_cylinder.verts.getAllocation().getBuffer();
    vkCmdBindVertexBuffers(context.getCommandBuffer(), 0, 2, vbs, offsets);
    context.bindIndexBuffer(m_cylinder.indices);

    context.drawIndexed(m_cylinder.indices.getIndexCount(), m_count);
  }
}
} // namespace AltheaEngine