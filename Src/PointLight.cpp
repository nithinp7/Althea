#include "PointLight.h"

#include "Application.h"

#include <glm/gtc/constants.hpp>

#include <memory>

namespace AltheaEngine {

// TODO: Refactor out into generalized storage buffer class...

PointLightCollection::PointLightCollection(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    size_t lightCount,
    bool createShadowMap)
    : _dirty(true),
      _lights(),
      _buffer(
          app,
          commandBuffer,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          lightCount * sizeof(PointLight),
          app.getPhysicalDeviceProperties()
              .limits.minStorageBufferOffsetAlignment),
      _sphere(app, commandBuffer) {
  this->_lights.resize(lightCount);

  if (createShadowMap) {
    // TODO: Configure shadowmap resolution
    this->_shadowMap = RenderTargetCollection(
        app,
        commandBuffer,
        VkExtent2D{256, 256},
        lightCount,
        RenderTargetFlags::SceneCaptureCube |
            RenderTargetFlags::EnableDepthTarget);
  }
}

void PointLightCollection::setLight(uint32_t lightId, const PointLight& light) {
  this->_lights[lightId] = light;

  this->_dirty = true;
}

void PointLightCollection::updateResource(const FrameContext& frame) {
  // if (!this->_dirty) {
  //   return;
  // }

  this->_scratchBytes.resize(this->_buffer.getSize());
  std::memcpy(
      this->_scratchBytes.data(),
      this->_lights.data(),
      this->_buffer.getSize());
  this->_buffer.updateData(frame.frameRingBufferIndex, this->_scratchBytes);

  this->_dirty = false;
}

void PointLightCollection::transitionToAttachment(
    VkCommandBuffer commandBuffer) {
  this->_shadowMap.transitionToAttachment(commandBuffer);
}

void PointLightCollection::transitionToTexture(VkCommandBuffer commandBuffer) {
  this->_shadowMap.transitionToTexture(commandBuffer);
}

void PointLightCollection::setupPointLightMeshSubpass(
    SubpassBuilder& subpassBuilder,
    uint32_t colorAttachment,
    uint32_t depthAttachment,
    VkDescriptorSetLayout globalDescriptorSetLayout) const {
  subpassBuilder.colorAttachments.push_back(colorAttachment);
  subpassBuilder.depthAttachment = depthAttachment;

  subpassBuilder
      .pipelineBuilder
      // TODO: This is a hack to workaround incorrect winding of sphere
      // triangle indices - fix that instead of disabling backface culling
      .addVertexInputBinding<glm::vec3>(VK_VERTEX_INPUT_RATE_VERTEX)
      .addVertexAttribute(VertexAttributeType::VEC3, 0)

      // Vertex shader
      .addVertexShader(GEngineDirectory + "/Shaders/LightMesh.vert")
      // Fragment shader
      .addFragmentShader(GEngineDirectory + "/Shaders/LightMesh.frag")

      // Pipeline resource layouts
      .layoutBuilder
      // Global resources (view, projection, environment ma)
      .addDescriptorSet(globalDescriptorSetLayout);
}

void PointLightCollection::draw(const DrawContext& context) const {
  context.bindDescriptorSets();
  context.bindVertexBuffer(this->_sphere.vertexBuffer);
  context.bindIndexBuffer(this->_sphere.indexBuffer);
  vkCmdDrawIndexed(
      context.getCommandBuffer(),
      this->_sphere.indexBuffer.getIndexCount(),
      static_cast<uint32_t>(this->_lights.size()),
      0,
      0,
      0);
}

PointLightCollection::Sphere::Sphere(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer) {
  constexpr uint32_t resolution = 50;
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

  this->indexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));
  this->vertexBuffer =
      VertexBuffer<glm::vec3>(app, commandBuffer, std::move(vertices));
}
} // namespace AltheaEngine