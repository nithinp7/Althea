#pragma once

#include "ConfigParser.h"
#include "Primitive.h"

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include <CesiumGltf/Model.h>
#include <vulkan/vulkan.h>
#include <array>

class Model {
public:
  Model(
      const VkDevice& device, 
      const VkPhysicalDevice& physicalDevice,
      const std::string& path);
  void render(const VkCommandBuffer& commandBuffer) const;
  void destroy(const VkDevice& device);
private:
  CesiumGltf::Model _model;
  std::vector<Primitive> _primitives;
};

class ModelManager {
private:
  std::vector<Model> _models;
public:
  ModelManager(
      const VkDevice& device, 
      const VkPhysicalDevice& physicalDevice, 
      const std::string& graphicsPipelineName,
      const ConfigParser& configParser);
  void render(const VkCommandBuffer& commandBuffer) const;
  void destroy(const VkDevice& device);
};

