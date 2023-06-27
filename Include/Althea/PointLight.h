#pragma once

#include "Library.h"

#include "BufferUtilities.h"
#include "DynamicBuffer.h"
#include "FrameContext.h"
#include "RenderTarget.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace AltheaEngine {
class Application;

struct ALTHEA_API PointLight {
  alignas(16) glm::vec3 position;
  alignas(16) glm::vec3 emmision;
};

// TODO: Provide alternative dynamic point light collection, that can be updated every frame

class ALTHEA_API PointLightCollection {
public:
  PointLightCollection() = default;
  PointLightCollection(
      const Application& app,
      VkCommandBuffer commandBuffer,
      size_t lightCount,
      bool createShadowMap);
  void setLight(uint32_t lightId, const PointLight& light);
  void updateResource(const FrameContext& frame);

  void transitionToAttachment(VkCommandBuffer commandBuffer);
  void transitionToTexture(VkCommandBuffer commandBuffer);
  
  size_t getByteSize() const {
    return this->_buffer.getSize();
  }

  const BufferAllocation& getAllocation() const {
    return this->_buffer.getAllocation();
  }

  const PointLight& getLight(uint32_t lightIndex) const {
    return this->_lights[lightIndex];
  }

  size_t getCount() const {
    return this->_lights.size();
  }

  Image& getShadowMapImage() { return this->_shadowMap.getDepthImage(); }

  const Image& getShadowMapImage() const { return this->_shadowMap.getDepthImage(); }

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
  
  Sampler& getShadowMapSampler() {
    return this->_shadowMap.getDepthSampler();
  }

  const Sampler& getShadowMapSampler() const {
    return this->_shadowMap.getDepthSampler();
  }

private:
  bool _dirty;
  std::vector<PointLight> _lights;
  DynamicBuffer _buffer;

  RenderTargetCollection _shadowMap;

  std::vector<std::byte> _scratchBytes;
};
} // namespace AltheaEngine