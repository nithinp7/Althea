#pragma once

#include <Althea/Application.h>
#include <Althea/DeferredRendering.h>

namespace AltheaEngine {
class Floor : public IGBufferSubpass {
public:
  Floor() = default;

  // IGBufferSubpass impl
  void registerGBufferSubpass(GraphicsPipelineBuilder& builder) const override;
  void beginGBufferSubpass(
      const DrawContext& context,
      BufferHandle globalResourcesHandle,
      UniformHandle globalUniformsHandle) override;

  float m_floorHeight = 0.0f;
  float m_floorHalfWidth = 100.0f;
};
} // namespace AltheaEngine