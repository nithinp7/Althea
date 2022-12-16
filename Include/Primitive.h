#pragma once

#include "Texture.h"

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

#include <cstdint>

#include <unordered_map>
#include <memory>

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
struct Texture;
struct TextureInfo;
} // namespace CesiumGltf

namespace AltheaEngine {
class Application;

struct Vertex {
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 uvs[MAX_UV_COORDS]{};

  static VkVertexInputBindingDescription getBindingDescription();
  static std::array<VkVertexInputAttributeDescription, 2 + MAX_UV_COORDS> 
      getAttributeDescriptions();
};

struct PrimitiveConstants {
  glm::vec4 debugColor = glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);
  int32_t baseTextureCoordinateIndex{};
  int32_t normalMapTextureCoordinateIndex{};
};

struct TextureSlots {
  std::shared_ptr<Texture> pBaseTexture;
  std::shared_ptr<Texture> pNormalMapTexture;

  void fillEmptyWithDefaults();
};

class Primitive {
public: 
  static std::array<VkDescriptorSetLayoutBinding, 4> getBindings();
  static std::array<VkDescriptorPoolSize, 4> getPoolSizes(uint32_t descriptorCount);

private:
  VkDevice _device;

  glm::mat4 _relativeTransform;

  std::vector<Vertex> _vertices;
  std::vector<uint32_t> _indices;

  PrimitiveConstants _constants;

  TextureSlots _textureSlots;

  VkBuffer _vertexBuffer;
  VkBuffer _indexBuffer;
  std::vector<VkBuffer> _uniformBuffers;

  VkDeviceMemory _vertexBufferMemory;
  VkDeviceMemory _indexBufferMemory;
  std::vector<VkDeviceMemory> _uniformBuffersMemory;
  
  std::vector<VkDescriptorSet> _descriptorSets;

  bool _needsDestruction = true;
public:
  Primitive(
      const Application& app,
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const glm::mat4& nodeTransform);
  Primitive(Primitive&& rhs) noexcept;
  Primitive(const Primitive& rhs) = delete;
  ~Primitive() noexcept;
  void updateUniforms(
      const glm::mat4& parentTransform, 
      const glm::mat4& view, 
      const glm::mat4& projection, 
      uint32_t currentFrame) const;
  void assignDescriptorSets(std::vector<VkDescriptorSet>& availableDescriptorSets);
  void render(
      const VkCommandBuffer& commandBuffer, 
      const VkPipelineLayout& pipelineLayout, 
      uint32_t currentFrame) const;
};
} // namespace AltheaEngine
