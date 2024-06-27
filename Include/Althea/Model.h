#pragma once

#include "ConfigParser.h"
#include "DrawContext.h"
#include "FrameContext.h"
#include "GlobalHeap.h"
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

struct Node {
  glm::mat4 currentTransform = glm::mat4(1.0f);
  int32_t meshIdx = -1;
};

struct Mesh {
  uint32_t primitiveStartIdx = 0;
  uint32_t primitiveCount = 0;
};

class ALTHEA_API Model {
public:
  Model(const Model& rhs) = delete;
  Model(Model&& rhs) = default;

  // TODO: Create version that can take regular VkCommandBuffer
  Model(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const std::string& path,
      DescriptorSetAllocator* pMaterialAllocator = nullptr);
  void registerToHeap(GlobalHeap& heap);

  void setModelTransform(const glm::mat4& modelTransform);
  size_t getPrimitivesCount() const;
  size_t getAnimationCount() const { return _model.animations.size(); }
  const std::string& getAnimationName(int32_t i) const {
    return _model.animations[i].name;
  }
  int32_t getAnimationIndex(const std::string& name) const {
    for (int32_t i = 0; i < _model.animations.size(); ++i)
      if (_model.animations[i].name == name)
        return i;
    return -1;
  }

  void updateAnimation(float deltaTime);
  void startAnimation(int32_t idx, bool bLoop);
  void stopAllAnimations() {
    _activeAnimation = -1;
    _animationTime = 0.0f;
    _animationLooping = false;
  }

  void draw(const DrawContext& context) const;

  const std::vector<Primitive>& getPrimitives() const {
    return this->_primitives;
  }

  std::vector<Primitive>& getPrimitives() { return this->_primitives; }

  void createConstantBuffers(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap);

private:
  CesiumGltf::Model _model;
  std::vector<Node> _nodes;
  std::vector<Mesh> _meshes;
  std::vector<Primitive> _primitives;

  glm::mat4 _modelTransform;
  int32_t _activeAnimation = -1;
  float _animationTime = 0.0f;
  bool _animationLooping = false;

  void _updateTransforms(int32_t nodeIdx, const glm::mat4& transform);
  void _drawNode(const DrawContext& context, int32_t nodeIdx) const;
  void _loadNode(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      int32_t nodeIdx,
      const glm::mat4& parentTransform,
      DescriptorSetAllocator* pMaterialAllocator);
};
} // namespace AltheaEngine
