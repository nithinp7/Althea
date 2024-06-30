#pragma once

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

struct ALTHEA_API Vertex {
  glm::vec3 position{};
  glm::vec3 tangent{};
  glm::vec3 bitangent{};
  glm::vec3 normal{};
  glm::vec2 uvs[MAX_UV_COORDS]{};
  glm::vec4 weights{};
  glm::u16vec4 joints{};
};

struct ALTHEA_API AABB {
  glm::vec3 min{};
  glm::vec3 max{};
};

// TODO: validate alignment, may be too big for inline block
struct ALTHEA_API PrimitiveConstants {
  glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  glm::vec4 emissiveFactor{};

  int32_t baseTextureCoordinateIndex{};
  int32_t normalMapTextureCoordinateIndex{};
  int32_t metallicRoughnessTextureCoordinateIndex{};
  int32_t occlusionTextureCoordinateIndex{};

  int32_t emissiveTextureCoordinateIndex{};
  float normalScale = 1.0f;
  float metallicFactor = 0.0f;
  float roughnessFactor = 1.0f;

  float occlusionStrength = 1.0f;
  float alphaCutoff = 0.5f;
  uint32_t baseTextureHandle;
  uint32_t normalTextureHandle;

  uint32_t metallicRoughnessTextureHandle;
  uint32_t occlusionTextureHandle;
  uint32_t emissiveTextureHandle;
  uint32_t vertexBufferHandle;

  uint32_t indexBufferHandle;
  uint32_t isSkinned; // TODO: turn this into a flag bitmask
  uint32_t padding2;
  uint32_t padding3;
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

  const glm::mat4& getTransform() const {
    return this->_transform;
  }

  BufferHandle getConstantBufferHandle() const {
    return _constantBuffer.getHandle();
  }

private:
  void registerToHeap(GlobalHeap& heap);
  void createConstantBuffer(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap);

  VkDevice _device;

  glm::mat4 _transform;
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
      const glm::mat4& nodeTransform);

  void setTransform(const glm::mat4& transform) { _transform = transform; }
  VkFrontFace getFrontFace() const {
    return this->_flipFrontFace ? VK_FRONT_FACE_CLOCKWISE
                                : VK_FRONT_FACE_COUNTER_CLOCKWISE;
  }
  void draw(const DrawContext& context) const;
};
} // namespace AltheaEngine
