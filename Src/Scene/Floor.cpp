#include <Althea/Scene/Floor.h>

namespace {
struct FloorPush {
  uint32_t globalUniforms;
  float floorHeight;
  float halfWidth;
};
} // namespace

namespace AltheaEngine {

// IGBufferSubpass impl
void Floor::registerGBufferSubpass(GraphicsPipelineBuilder& builder) const {
  builder.addVertexShader(GEngineDirectory + "/Shaders/Misc/Floor.vert")
      .addFragmentShader(
          GEngineDirectory + "/Shaders/GBuffer/GBufferPassThrough.frag")

      .layoutBuilder.addPushConstants<FloorPush>(VK_SHADER_STAGE_ALL);
}

void Floor::beginGBufferSubpass(
    const DrawContext& context,
    BufferHandle globalResources,
    UniformHandle globalUniforms) {
  const FrameContext& frame = context.getFrame();

  {
    FloorPush constants{};
    constants.globalUniforms = globalUniforms.index;
    constants.floorHeight = m_floorHeight;
    constants.halfWidth = m_floorHalfWidth;

    context.updatePushConstants(constants, 0);
    context.bindDescriptorSets();

    context.draw(6);
  }
}
} // namespace AltheaEngine