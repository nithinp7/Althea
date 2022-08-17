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

class Application;

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
struct Texture;
struct TextureInfo;
} // namespace CesiumGltf

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;

  static VkVertexInputBindingDescription getBindingDescription();
  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct TextureCoordinateSet {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

class Primitive {
private:
  VkDevice _device;

  glm::mat4 _relativeTransform;

  std::vector<Vertex> _vertices;
  std::vector<uint32_t> _indices;

  VkBuffer _vertexBuffer;
  VkBuffer _indexBuffer;
  std::vector<VkBuffer> _uniformBuffers;

  VkDeviceMemory _vertexBufferMemory;
  VkDeviceMemory _indexBufferMemory;
  std::vector<VkDeviceMemory> _uniformBuffersMemory;

  std::vector<VkDescriptorSet> _descriptorSets;

  std::unique_ptr<Texture> _pBaseTexture;

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

private:
  void _loadTexture(
      const CesiumGltf::Model& model,
      const CesiumGltf::TextureInfo& texture,
      std::unordered_map<const CesiumGltf::Texture*, Texture>& textureMap,
      std::unordered_map<uint32_t, TextureCoordinateSet>& textureCoordinateAttributes); 
};