#include "PointLight.h"

#include "Application.h"

#include <memory>

namespace AltheaEngine {

// TODO: Refactor out into generalized storage buffer class...

PointLightCollection::PointLightCollection(
    const Application& app,
    VkCommandBuffer commandBuffer,
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
              .limits.minStorageBufferOffsetAlignment) {
  this->_lights.resize(lightCount);
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
} // namespace AltheaEngine