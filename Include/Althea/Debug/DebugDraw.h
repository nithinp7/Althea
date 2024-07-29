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

class DebugDrawCapsules : public IGBufferSubpass {
public:
  DebugDrawCapsules() = default;
  DebugDrawCapsules(Application& app, VkCommandBuffer commandBuffer, uint32_t maxCapacity, bool wireFrame);

  // IGBufferSubpass impl
  void registerGBufferSubpass(GraphicsPipelineBuilder& builder) const override;
  void beginGBufferSubpass(
      const DrawContext& context,
      BufferHandle globalResourcesHandle,
      UniformHandle globalUniformsHandle) override;

  void reset() { m_count = 0; }
  bool addCapsule(const glm::vec3& a, const glm::vec3& b, float radius, uint32_t color) {
    if (m_count < m_capsules.getVertexCount()) {
      m_capsules.setVertex({glm::mat4(1.0f), a, b, radius, color}, m_count++);
      return true;
    }
    return false;
  }

  bool addCapsule(const glm::mat4& model, const glm::vec3& a, const glm::vec3& b, float radius, uint32_t color) {
    if (m_count < m_capsules.getVertexCount()) {
      m_capsules.setVertex({ model, a, b, radius, color }, m_count++);
      return true;
    }
    return false;
  }

private:
  struct Mesh {
    VertexBuffer<glm::vec3> verts;
    IndexBuffer indices;
  };
  Mesh m_sphere;
  Mesh m_cylinder;

  bool m_bWireframe;

  struct CapsuleInst {
    glm::mat4 model;
    glm::vec3 a;
    glm::vec3 b;
    float radius;
    uint32_t color;
  };
  DynamicVertexBuffer<CapsuleInst> m_capsules;
  uint32_t m_count = 0;
};
} // namespace AltheaEngine