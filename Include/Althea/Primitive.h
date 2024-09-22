#pragma once

#include "Common/InstanceDataCommon.h"
#include "ConstantBuffer.h"
#include "DrawContext.h"
#include "GlobalHeap.h"
#include "IndexBuffer.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"
#include "Texture.h"
#include "VertexBuffer.h"

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
struct Texture;
struct TextureInfo;
} // namespace CesiumGltf

namespace AltheaEngine {
class Application;
class GraphicsPipelineBuilder;
class DescriptorSet;
class DescriptorSetAllocator;

struct ALTHEA_API AABB {
  glm::vec3 min{};
  glm::vec3 max{};
};

class ALTHEA_API Primitive {
public:
  static void buildPipeline(GraphicsPipelineBuilder& builder);

  const PrimitiveConstants& getConstants() const { return m_constantBuffer.getConstants(); }

  const VertexBuffer<Vertex>& getVertexBuffer() const {
    return m_vertexBuffer;
  }

  const IndexBuffer& getIndexBuffer() const { return m_indexBuffer; }

  const std::vector<Vertex>& getVertices() const {
    return m_vertexBuffer.getVertices();
  }

  const std::vector<uint32_t>& getIndices() const {
    return m_indexBuffer.getIndices();
  }

  AABB computeWorldAABB() const;

  const AABB& getAABB() const { return m_aabb; }

  BufferHandle getConstantBufferHandle() const {
    return m_constantBuffer.getHandle();
  }

  uint32_t getNodeIdx() const { return m_constantBuffer.getConstants().nodeIdx; }
  bool isSkinned() const { return m_constantBuffer.getConstants().isSkinned != 0; }

private:
  bool m_flipFrontFace = false;

  ConstantBuffer<PrimitiveConstants> m_constantBuffer;

  VertexBuffer<Vertex> m_vertexBuffer;
  IndexBuffer m_indexBuffer;

  AABB m_aabb;

public:
  Primitive(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap,
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const std::vector<Material>& materialMap,
      BufferHandle handle,
      uint32_t nodeIdx);

  VkFrontFace getFrontFace() const {
    return m_flipFrontFace ? VK_FRONT_FACE_CLOCKWISE
                                : VK_FRONT_FACE_COUNTER_CLOCKWISE;
  }
};
} // namespace AltheaEngine
