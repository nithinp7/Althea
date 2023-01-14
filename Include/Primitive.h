#pragma once

#include "DrawContext.h"
#include "IndexBuffer.h"
#include "Material.h"
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

struct Vertex {
  glm::vec3 position{};
  glm::vec3 tangent{};
  glm::vec3 bitangent{};
  glm::vec3 normal{};
  glm::vec2 uvs[MAX_UV_COORDS]{};
};

// TODO: validate alignment, may be too big for inline block
struct PrimitiveConstants {
  glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  glm::vec3 emissiveFactor{};

  int32_t baseTextureCoordinateIndex{};
  int32_t normalMapTextureCoordinateIndex{};
  int32_t metallicRoughnessTextureCoordinateIndex{};
  int32_t occlusionTextureCoordinateIndex{};
  int32_t emissiveTextureCoordinateIndex{};

  float normalScale = 1.0f;
  float metallicFactor = 1.0f;
  float roughnessFactor = 1.0f;
  float occlusionStrength = 1.0f;

  float alphaCutoff = 0.5f;
};

struct TextureSlots {
  std::shared_ptr<Texture> pBaseTexture;
  std::shared_ptr<Texture> pNormalMapTexture;
  std::shared_ptr<Texture> pMetallicRoughnessTexture;
  std::shared_ptr<Texture> pOcclusionTexture;
  std::shared_ptr<Texture> pEmissiveTexture;

  void fillEmptyWithDefaults();
};

class Primitive {
public:
  static void buildPipeline(GraphicsPipelineBuilder& builder);

private:
  VkDevice _device;

  glm::mat4 _relativeTransform;
  bool _flipFrontFace = false;

  PrimitiveConstants _constants;
  TextureSlots _textureSlots;

  VertexBuffer<Vertex> _vertexBuffer;
  IndexBuffer _indexBuffer;
  Material _material;

public:
  Primitive(
      const Application& app,
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const glm::mat4& nodeTransform,
      DescriptorSetAllocator& materialAllocator);
  Primitive(const Primitive& rhs) = delete;
  Primitive& operator=(const Primitive& rhs) = delete;
  Primitive(Primitive&& rhs) = delete;

  void draw(const DrawContext& context) const;
};
} // namespace AltheaEngine
