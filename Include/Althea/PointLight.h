#pragma once

#include "BufferUtilities.h"
#include "DrawContext.h"
#include "DynamicBuffer.h"
#include "FrameBuffer.h"
#include "FrameContext.h"
#include "GlobalHeap.h"
#include "IndexBuffer.h"
#include "Library.h"
#include "Model.h"
#include "PerFrameResources.h"
#include "RenderPass.h"
#include "RenderTarget.h"
#include "SingleTimeCommandBuffer.h"
#include "VertexBuffer.h"
#include "StructuredBuffer.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace AltheaEngine {
class Application;

struct ALTHEA_API PointLight {
  alignas(16) glm::vec3 position;
  alignas(16) glm::vec3 emission;
};

// TODO: Provide alternative dynamic point light collection, that can be updated
// every frame

class ALTHEA_API PointLightCollection {
public:
  PointLightCollection() = default;
  PointLightCollection(
      Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap,
      size_t lightCount,
      bool createShadowMap,
      BufferHandle primConstants);
  void setLight(uint32_t lightId, const PointLight& light);
  void updateResource(const FrameContext& frame);

  void setupPointLightMeshSubpass(
      SubpassBuilder& subpassBuilder,
      uint32_t colorAttachment,
      uint32_t depthAttachment,
      VkDescriptorSetLayout globalDescriptorSetLayout) const;
  void drawShadowMaps(
      Application& app,
      VkCommandBuffer commandBuffer,
      const FrameContext& frame,
      const std::vector<Model>& models,
      VkDescriptorSet globalSet = VK_NULL_HANDLE);
  void draw(const DrawContext& context) const;

  size_t getByteSize() const { return this->_buffer.getSize(); }

  const BufferAllocation& getAllocation() const {
    return this->_buffer.getAllocation();
  }

  const PointLight& getLight(uint32_t lightIndex) const {
    return this->_lights[lightIndex];
  }

  size_t getCount() const { return this->_lights.size(); }

  const VkExtent2D& getShadowMapExtent() const {
    return this->_shadowMap.getExtent();
  }

  Image& getShadowMapImage() { return this->_shadowMap.getDepthImage(); }

  const Image& getShadowMapImage() const {
    return this->_shadowMap.getDepthImage();
  }

  ImageView& getShadowMapTargetView(uint32_t targetIndex) {
    return this->_shadowMap.getTargetDepthView(targetIndex);
  }

  const ImageView& getShadowMapTargetView(uint32_t targetIndex) const {
    return this->_shadowMap.getTargetDepthView(targetIndex);
  }

  ImageView& getShadowMapArrayView() {
    return this->_shadowMap.getDepthTextureArrayView();
  }

  const ImageView& getShadowMapArrayView() const {
    return this->_shadowMap.getDepthTextureArrayView();
  }

  Sampler& getShadowMapSampler() { return this->_shadowMap.getDepthSampler(); }

  const Sampler& getShadowMapSampler() const {
    return this->_shadowMap.getDepthSampler();
  }

  RenderPass& getShadowMapPass() { return this->_shadowPass; }

private:
  bool _dirty;
  bool _useShadowMaps;
  std::vector<PointLight> _lights;
  DynamicBuffer _buffer;

  struct PointLightConstants {
    glm::mat4 projection;
    glm::mat4 inverseProjection;
    glm::mat4 views[6];
    glm::mat4 inverseViews[6];
  };
  StructuredBuffer<PointLightConstants> _pointLightConstants;

  // Sphere VB for visualizing the point lights
  struct Sphere {
    VertexBuffer<glm::vec3> vertexBuffer;
    IndexBuffer indexBuffer;

    Sphere() = default;
    Sphere(const Application& app, SingleTimeCommandBuffer& commandBuffer);
  };
  Sphere _sphere{};

  // Resources needed for shadow mapping, if it is enabled
  RenderTargetCollection _shadowMap;
  RenderPass _shadowPass;
  std::vector<FrameBuffer> _shadowFrameBuffers;

  std::vector<std::byte> _scratchBytes;
};
} // namespace AltheaEngine