#pragma once

#include "Library.h"
#include "VertexBuffer.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace AltheaEngine {

class Application;
class IndexBuffer;

class ALTHEA_API ShapeUtilities {
public:
  static void createSphere(
      Application& app,
      VkCommandBuffer commandBuffer,
      VertexBuffer<glm::vec3>& vertexBuffer,
      IndexBuffer& indexBuffer,
      uint32_t resolution,
      float radius = 1.0f,
      bool wireframe = false);
  static void createCylinder(
      Application& app,
      VkCommandBuffer commandBuffer, 
      VertexBuffer<glm::vec3>& vertexBuffer,
      IndexBuffer& indexBuffer,
      uint32_t resolution,
      bool wireframe = false);
};
} // namespace AltheaEngine