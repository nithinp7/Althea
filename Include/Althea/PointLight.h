#pragma once

#include "Library.h"

#include "BufferUtilities.h"
#include "DynamicBuffer.h"
#include "FrameContext.h"

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
      std::vector<PointLight>&& lights);
  PointLightCollection(
      const Application& app,
      VkCommandBuffer commandBuffer,
      size_t lightCount);
  void setLight(uint32_t lightId, const PointLight& light);
  void updateResource(const FrameContext& frame);

  size_t getByteSize() const {
    return this->_buffer.getSize();
  }

  const BufferAllocation& getAllocation() const {
    return this->_buffer.getAllocation();
  }

  size_t getCount() const {
    return this->_lights.size();
  }

private:
  bool _dirty;
  std::vector<PointLight> _lights;
  DynamicBuffer _buffer;

  std::vector<std::byte> _scratchBytes;
};
} // namespace AltheaEngine