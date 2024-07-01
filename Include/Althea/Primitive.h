#pragma once

#include "Common/GltfCommon.h"
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

struct ALTHEA_API TextureSlots {
  std::shared_ptr<Texture> pBaseTexture;
  std::shared_ptr<Texture> pNormalMapTexture;
  std::shared_ptr<Texture> pMetallicRoughnessTexture;
  std::shared_ptr<Texture> pOcclusionTexture;
  std::shared_ptr<Texture> pEmissiveTexture;

  void fillEmptyWithDefaults();
};

class ALTHEA_API Primitive {
public:
  static void buildPipeline(GraphicsPipelineBuilder& builder);

  const PrimitiveConstants& getConstants() const { return this->_constants; }

  const VertexBuffer<Vertex>& getVertexBuffer() const {
    return this->_vertexBuffer;
  }

  const IndexBuffer& getIndexBuffer() const { return this->_indexBuffer; }

  const std::vector<Vertex>& getVertices() const {
    return this->_vertexBuffer.getVertices();
  }

  const std::vector<uint32_t>& getIndices() const {
    return this->_indexBuffer.getIndices();
  }

  const TextureSlots& getTextures() const { return this->_textureSlots; }

  AABB computeWorldAABB() const;

  const AABB& getAABB() const { return this->_aabb; }

  BufferHandle getConstantBufferHandle() const {
    return _constantBuffer.getHandle();
  }

  uint32_t getNodeIdx() const { return _constants.nodeIdx; }
  bool isSkinned() const { return _constants.isSkinned != 0; }

private:
  void registerToHeap(GlobalHeap& heap);
  void createConstantBuffer(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap);

  VkDevice _device;

  bool _flipFrontFace = false;

  PrimitiveConstants _constants;
  ConstantBuffer<PrimitiveConstants> _constantBuffer;
  TextureSlots _textureSlots;

  VertexBuffer<Vertex> _vertexBuffer;
  IndexBuffer _indexBuffer;

  AABB _aabb;

public:
  Primitive(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap,
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      BufferHandle handle,
      uint32_t nodeIdx);

  VkFrontFace getFrontFace() const {
    return this->_flipFrontFace ? VK_FRONT_FACE_CLOCKWISE
                                : VK_FRONT_FACE_COUNTER_CLOCKWISE;
  }
};
} // namespace AltheaEngine
