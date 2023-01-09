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

struct PrimitiveConstants {
  int32_t baseTextureCoordinateIndex{};
  int32_t normalMapTextureCoordinateIndex{};
  float normalScale = 1.0f;
};

struct TextureSlots {
  std::shared_ptr<Texture> pBaseTexture;
  std::shared_ptr<Texture> pNormalMapTexture;

  void fillEmptyWithDefaults();
};

class Primitive {
public:
  static void buildPipeline(GraphicsPipelineBuilder& builder);

private:
  VkDevice _device;

  glm::mat4 _relativeTransform;
  bool _flipFrontFace = false;

  std::vector<Vertex> _vertices;
  std::vector<uint32_t> _indices;

  PrimitiveConstants _constants;

  TextureSlots _textureSlots;

  VertexBuffer _vertexBuffer;
  IndexBuffer _indexBuffer;

  std::vector<VkBuffer> _uniformBuffers;
  std::vector<VkDeviceMemory> _uniformBuffersMemory;

  std::unique_ptr<Material> _pMaterial;

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
  ~Primitive();

  void updateUniforms(
      const glm::mat4& parentTransform,
      const glm::mat4& view,
      const glm::mat4& projection,
      uint32_t currentFrame) const;
  void draw(const DrawContext& context) const;
};
} // namespace AltheaEngine
