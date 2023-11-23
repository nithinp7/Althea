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
  static void createSphere(Application& app, VkCommandBuffer commandBuffer, VertexBuffer<glm::vec3>& vertexBuffer, IndexBuffer& indexBuffer);
};
} // namespace AltheaEngine