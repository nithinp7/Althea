#pragma once

#include "ConfigParser.h"
#include "DrawContext.h"
#include "FrameContext.h"
#include "Library.h"
#include "Primitive.h"
#include "SingleTimeCommandBuffer.h"

#include <CesiumGltf/Model.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace AltheaEngine {
class Application;
class GraphicsPipeline;
class DescriptorSetAllocator;

class ALTHEA_API Model {
public:
  Model(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const std::string& path,
      DescriptorSetAllocator& materialAllocator);

  size_t getPrimitivesCount() const;
  void draw(const DrawContext& context) const;

private:
  CesiumGltf::Model _model;
  std::vector<Primitive> _primitives;

  void _loadNode(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const CesiumGltf::Model& model,
      const CesiumGltf::Node& node,
      const glm::mat4& transform,
      DescriptorSetAllocator& materialAllocator);
};
} // namespace AltheaEngine
