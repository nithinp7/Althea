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
    uint32_t resolution,
    float radius,
    bool wireframe) {
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

      vertices.emplace_back(
          radius * cosPhi * cosTheta,
          radius * sinPhi,
          radius * -cosPhi * sinTheta);

      if (j < resolution / 2 - 1) {
        if (wireframe) {
          indices.push_back(sphereUvIndexToVertIndex(i, j));
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j));

          indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j + 1));
        } else {
          indices.push_back(sphereUvIndexToVertIndex(i, j));
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j + 1));

          indices.push_back(sphereUvIndexToVertIndex(i, j));
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j + 1));
          indices.push_back(sphereUvIndexToVertIndex(i, j + 1));
        }
      } else {
        if (wireframe) {
          indices.push_back(sphereUvIndexToVertIndex(i, j));
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
          indices.push_back(cylinderVertsCount);
        } else {
          indices.push_back(sphereUvIndexToVertIndex(i, j));
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
          indices.push_back(cylinderVertsCount);
        }
      }

      if (j == 0) {
        if (wireframe) {
          indices.push_back(sphereUvIndexToVertIndex(i, j));
          indices.push_back(cylinderVertsCount + 1);
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
          indices.push_back(sphereUvIndexToVertIndex(i, j));
        } else {
          indices.push_back(sphereUvIndexToVertIndex(i, j));
          indices.push_back(cylinderVertsCount + 1);
          indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
        }
      }
    }
  }

  // Cap vertices
  vertices.emplace_back(0.0f, radius, 0.0f);
  vertices.emplace_back(0.0f, -radius, 0.0f);

  indexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));
  vertexBuffer =
      VertexBuffer<glm::vec3>(app, commandBuffer, std::move(vertices));
}

/*static*/
void ShapeUtilities::createCylinder(
    Application& app,
    VkCommandBuffer commandBuffer,
    VertexBuffer<glm::vec3>& vertexBuffer,
    IndexBuffer& indexBuffer,
    uint32_t resolution,
    bool wireframe) {
  // resolution count of points along circumference on each cap,
  // plus another point at the center of each cap.
  uint32_t vertexCount = resolution * 2 + 2;
  std::vector<glm::vec3> vertices;
  vertices.reserve(vertexCount);

  // resolution count of triangles on each cap,
  // 2 * resolution count of triangles on sides connecting caps.
  uint32_t triangleCount = 3 * resolution;
  uint32_t indexCount = 3 * triangleCount;
  std::vector<uint32_t> indices;
  indices.reserve(indexCount);

  uint32_t capCenter0Index = 2 * resolution;
  uint32_t capCenter1Index = capCenter0Index + 1;

  for (uint32_t i = 0; i < resolution; ++i) {
    float theta = i * 2.0f * glm::pi<float>() / resolution;
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    vertices.emplace_back(cosTheta, sinTheta, 0.0f);
    vertices.emplace_back(cosTheta, sinTheta, 1.0f);

    // point on cap 0
    uint32_t p00 = 2 * i;
    // corresponding point on cap 1
    uint32_t p10 = p00 + 1;

    uint32_t j = (i + 1) % resolution;
    // next point on cap 0
    uint32_t p01 = 2 * j;
    // next point on cap 1
    uint32_t p11 = p01 + 1;

    // triangle on cap 0
    if (wireframe) {
      indices.push_back(capCenter0Index);
      indices.push_back(p01);
      indices.push_back(p01);
      indices.push_back(p00);
    } else {
      indices.push_back(capCenter0Index);
      indices.push_back(p01);
      indices.push_back(p00);
    }

    // triangle on cap 1
    if (wireframe) {
      indices.push_back(capCenter1Index);
      indices.push_back(p10);
      indices.push_back(p10);
      indices.push_back(p11);
    } else {
      indices.push_back(capCenter1Index);
      indices.push_back(p10);
      indices.push_back(p11);
    }

    // two triangles from quad on cylinder side
    if (wireframe) {
      indices.push_back(p00);
      indices.push_back(p10);

      indices.push_back(p10);
      indices.push_back(p11);
    } else {
      indices.push_back(p00);
      indices.push_back(p11);
      indices.push_back(p10);

      indices.push_back(p00);
      indices.push_back(p01);
      indices.push_back(p11);
    }
  }

  vertices.emplace_back(0.0f, 0.0f, 0.0f);
  vertices.emplace_back(0.0f, 0.0f, 1.0f);

  indexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));
  vertexBuffer =
      VertexBuffer<glm::vec3>(app, commandBuffer, std::move(vertices));
}
} // namespace AltheaEngine