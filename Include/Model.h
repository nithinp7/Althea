#pragma once

#include "ConfigParser.h"
#include "DrawContext.h"
#include "FrameContext.h"
#include "Primitive.h"

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

class Model {
public:
  Model(
      const Application& app,
      const std::string& name,
      DescriptorSetAllocator& materialAllocator);
  Model(const Model& rhs) = delete;
  Model& operator=(const Model& model) = delete;
  Model(Model&& rhs) = delete;

  size_t getPrimitivesCount() const;
  void draw(const DrawContext& context) const;

private:
  CesiumGltf::Model _model;
  std::vector<std::unique_ptr<Primitive>> _primitives;

  void _loadNode(
      const Application& app,
      const CesiumGltf::Model& model,
      const CesiumGltf::Node& node,
      const glm::mat4& transform,
      DescriptorSetAllocator& materialAllocator);
};
} // namespace AltheaEngine
