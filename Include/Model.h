#pragma once

#include "ConfigParser.h"
#include "Primitive.h"

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include <CesiumGltf/Model.h>
#include <vulkan/vulkan.h>
#include <array>

namespace AltheaEngine {
class Application;

class Model {
public:
  Model(
      const Application& app,
      const std::string& path);
  Model(Model&& rhs) = default;
  Model(const Model& rhs) =  delete;

  void updateUniforms(
      const glm::mat4& view, const glm::mat4& projection, uint32_t currentFrame) const;
  size_t getPrimitivesCount() const;
  void assignDescriptorSets(std::vector<VkDescriptorSet>& availableDescriptorSets);
  void render(
      const VkCommandBuffer& commandBuffer, 
      const VkPipelineLayout& pipelineLayout, 
      uint32_t currentFrame) const;
private:
  CesiumGltf::Model _model;
  std::vector<Primitive> _primitives;
  
  void _loadNode(
      const Application& app,
      const CesiumGltf::Model& model, 
      const CesiumGltf::Node& node, 
      const glm::mat4& transform);
};

class ModelManager {
private:
  std::vector<Model> _models;
public:
  ModelManager(
      const Application& app, 
      const std::string& graphicsPipelineName);
  ModelManager(const ModelManager& rhs) = delete;

  void updateUniforms(
      const glm::mat4& view, const glm::mat4& projection, uint32_t currentFrame) const;
  size_t getPrimitivesCount() const;
  void assignDescriptorSets(std::vector<VkDescriptorSet>& availableDescriptorSets);
  void render(
      const VkCommandBuffer& commandBuffer, 
      const VkPipelineLayout& pipelineLayout, 
      uint32_t currentFrame) const;
};
} // namespace AltheaEngine

