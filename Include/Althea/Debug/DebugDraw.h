#pragma once

#include <Althea/Debug/DebugDrawCommon.h>
#include <Althea/DynamicVertexBuffer.h>
#include <Althea/DeferredRendering.h>
#include <Althea/GraphicsPipeline.h>
#include <Althea/BindlessHandle.h>

#include <cstdint>

namespace AltheaEngine {
// Debug visualization scene
class DebugDrawLines : public IGBufferSubpass {
public:
  DebugDrawLines() = default;
  DebugDrawLines(Application& app, uint32_t maxLinesCapacity = 1000);

  // IGBufferSubpass impl
  void registerGBufferSubpass(GraphicsPipelineBuilder& builder) const override;
  void beginGBufferSubpass(
      const DrawContext& context,
      BufferHandle globalResourcesHandle,
      UniformHandle globalUniformsHandle) override;

  void reset() { m_lineCount = 0; }
  bool addLine(const glm::vec3& a, const glm::vec3& b, uint32_t color) {
    if (2 * m_lineCount < m_lines.getVertexCount()) {
      m_lines.setVertex({a, color}, 2 * m_lineCount);
      m_lines.setVertex({b, color}, 2 * m_lineCount + 1);
      ++m_lineCount;

      return true;
    }
    return false;
  }

private:
  struct DebugVert {
    alignas(16) glm::vec3 position;
    alignas(4) uint32_t color;
  };
  DynamicVertexBuffer<DebugVert> m_lines;
  uint32_t m_lineCount = 0;
};
} // namespace AltheaEngine