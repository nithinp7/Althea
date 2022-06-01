#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <cstdint>

class Application;

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
} // namespace CesiumGltf

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;

  static VkVertexInputBindingDescription getBindingDescription();
  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

class Primitive {
private:
  VkDevice _device;
  std::vector<Vertex> _vertices;
  std::vector<uint32_t> _indices;

  VkBuffer _vertexBuffer;
  VkBuffer _indexBuffer;
  std::vector<VkBuffer> _uniformBuffers;

  VkDeviceMemory _vertexBufferMemory;
  VkDeviceMemory _indexBufferMemory;
  std::vector<VkDeviceMemory> _uniformBuffersMemory;

  std::vector<VkDescriptorSet> _descriptorSets;
public:
  Primitive(
      const Application& app,
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive);
  ~Primitive();
  void updateUniforms(
      const glm::mat4& view, const glm::mat4& projection, uint32_t currentFrame) const;
  void assignDescriptorSets(std::vector<VkDescriptorSet>& availableDescriptorSets);
  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame) const;
};