#include "ShapeUtilities.h"

#include "Application.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"

#include <glm/gtc/constants.hpp>
#include <math.h>

#include <cstdint>
#include <vector>

namespace AltheaEngine {
/*static*/
void ShapeUtilities::createSphere(
    Application& app,
    VkCommandBuffer commandBuffer,
    VertexBuffer<glm::vec3>& vertexBuffer,
    IndexBuffer& indexBuffer,
    uint32_t resolution) {
  constexpr float maxPitch = 0.499f * glm::pi<float>();

  auto sphereUvIndexToVertIndex = [resolution](uint32_t i, uint32_t j) {
    i = i % resolution;
    return i * resolution / 2 + j;
  };

  std::vector<glm::vec3> vertices;
  std::vector<uint32_t> indices;

  // Verts from the cylinder mapping
  uint32_t cylinderVertsCount = resolution * resolution / 2;
  // Will include cylinder mapped verts and 2 cap verts
  vertices.reserve(cylinderVertsCount + 2);
  indices.reserve(3 * resolution * resolution);
  for (uint32_t i = 0; i < resolution; ++i) {
    float theta = i * 2.0f * glm::pi<float>() / resolution;
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);

    for (uint32_t j = 0; j < resolution / 2; ++j) {
      float phi = j * 2.0f * maxPitch / (resolution / 2) - maxPitch;
      float cosPhi = cos(phi);
      float sinPhi = sin(phi);

      vertices.emplace_back(cosPhi * cosTheta, sinPhi, -cosPhi * sinTheta);

      if (j < resolution / 2 - 1) {
        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j + 1));

        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j + 1));
        indices.push_back(sphereUvIndexToVertIndex(i, j + 1));
      } else {
        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
        indices.push_back(cylinderVertsCount);
      }

      if (j == 0) {
        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(cylinderVertsCount + 1);
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
      }
    }
  }

  // Cap vertices
  vertices.emplace_back(0.0f, 1.0f, 0.0f);
  vertices.emplace_back(0.0f, -1.0f, 0.0f);

  indexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));
  vertexBuffer = VertexBuffer<glm::vec3>(app, commandBuffer, std::move(vertices));
}
} // namespace AltheaEngine