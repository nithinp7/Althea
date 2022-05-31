#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <cstdint>

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
  std::vector<Vertex> _vertices;
  std::vector<uint32_t> _indices;

  VkBuffer _vertexBuffer;
  VkBuffer _indexBuffer;

  VkDeviceMemory _vertexBufferMemory;
  VkDeviceMemory _indexBufferMemory;
public:
  Primitive(
      const VkDevice& device,
      const VkPhysicalDevice& physicalDevice,
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive);
  void render(const VkCommandBuffer& commandBuffer) const;
  void destroy(const VkDevice& device);
};