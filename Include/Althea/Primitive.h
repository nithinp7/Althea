#pragma once

#include "DrawContext.h"
#include "IndexBuffer.h"
#include "Library.h"
#include "Material.h"
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

  float padding1;
  float padding2;
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
  // TODO: This is super hacky, need an actual, flexible way of managing
  // bindless indices
  static void resetPrimitiveIndexCount();

  static void buildPipeline(GraphicsPipelineBuilder& builder);
  static void buildMaterial(DescriptorSetLayoutBuilder& materialBuilder);

  const PrimitiveConstants& getConstants() const {
    return this->_constants;
  }

  const VertexBuffer<Vertex>& getVertexBuffer() const {
    return this->_vertexBuffer;
  }

  const IndexBuffer& getIndexBuffer() const {
    return this->_indexBuffer;
  }

  const std::vector<Vertex>& getVertices() const {
    return this->_vertexBuffer.getVertices();
  }

  const std::vector<uint32_t>& getIndices() const {
    return this->_indexBuffer.getIndices();
  }

  const TextureSlots& getTextures() const {
    return this->_textureSlots;
  }

  AABB computeWorldAABB() const;

  const AABB& getAABB() const {
    return this->_aabb;
  }

  glm::mat4 computeWorldTransform() const {
    return this->_modelTransform * this->_relativeTransform;
  }

  const glm::mat4& getRelativeTransform() const {
    return this->_relativeTransform;
  }

  int getPrimitiveIndex() const {
    return this->_primitiveIndex;
  }

private:
  VkDevice _device;

  glm::mat4 _modelTransform;
  glm::mat4 _relativeTransform;
  bool _flipFrontFace = false;

  int _primitiveIndex;

  PrimitiveConstants _constants;
  TextureSlots _textureSlots;

  VertexBuffer<Vertex> _vertexBuffer;
  IndexBuffer _indexBuffer;
  Material _material;

  AABB _aabb;

public:
  Primitive(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const glm::mat4& nodeTransform,
      DescriptorSetAllocator* pMaterialAllocator = nullptr);

  void setModelTransform(const glm::mat4& model);
  void draw(const DrawContext& context) const;
};
} // namespace AltheaEngine
