#pragma once

#include "ConfigParser.h"
#include "Primitive.h"

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include <CesiumGltf/Model.h>
#include <vulkan/vulkan.h>
#include <array>

class Application;

class Model {
public:
  Model(
      const Application& app,
      const std::string& path);
  void updateUniforms(
      const glm::mat4& view, const glm::mat4& projection, uint32_t currentFrame) const;
  size_t getPrimitivesCount() const;
  void assignDescriptorSets(std::vector<VkDescriptorSet>& availableDescriptorSets);
  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame) const;
private:
  CesiumGltf::Model _model;
  std::vector<Primitive> _primitives;
};

class ModelManager {
private:
  std::vector<Model> _models;
public:
  ModelManager(
      const Application& app, 
      const std::string& graphicsPipelineName);
  void updateUniforms(
      const glm::mat4& view, const glm::mat4& projection, uint32_t currentFrame) const;
  size_t getPrimitivesCount() const;
  void assignDescriptorSets(std::vector<VkDescriptorSet>& availableDescriptorSets);
  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame) const;
};

