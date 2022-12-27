#pragma once

#include "ConfigParser.h"
#include "Primitive.h"
#include "FrameContext.h"

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include <CesiumGltf/Model.h>
#include <vulkan/vulkan.h>
#include <array>
#include <memory>

namespace AltheaEngine {
class Application;
class GraphicsPipeline;

class Model {
public:
  Model(
      const Application& app,
      const std::string& name);
  Model(const Model& rhs) = delete;
  Model& operator=(const Model& model) = delete;
  Model(Model&& rhs) = delete;

  size_t getPrimitivesCount() const;
  void assignDescriptorSets(const Application& app, GraphicsPipeline& pipeline);
  void updateUniforms(
      const glm::mat4& view, const glm::mat4& projection, const FrameContext& frame) const;
  void draw(
      const VkCommandBuffer& commandBuffer, 
      const VkPipelineLayout& pipelineLayout, 
      const FrameContext& frame) const;
private:
  CesiumGltf::Model _model;
  std::vector<std::unique_ptr<Primitive>> _primitives;
  
  void _loadNode(
      const Application& app,
      const CesiumGltf::Model& model, 
      const CesiumGltf::Node& node, 
      const glm::mat4& transform);
};

class ModelManager {
private:
  std::vector<std::unique_ptr<Model>> _models;
public:
  ModelManager(
      const Application& app, 
      const std::string& graphicsPipelineName);
  ModelManager(const ModelManager& rhs) = delete;
  size_t getPrimitivesCount() const;
  void assignDescriptorSets(const Application& app, GraphicsPipeline& pipeline);
  void updateUniforms(
      const glm::mat4& view, const glm::mat4& projection, const FrameContext& frame) const;
  void draw(
      const VkCommandBuffer& commandBuffer, 
      const VkPipelineLayout& pipelineLayout, 
      const FrameContext& frame) const;
};
} // namespace AltheaEngine

