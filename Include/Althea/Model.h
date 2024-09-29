#pragma once

#include "ConfigParser.h"
#include "DrawContext.h"
#include "DynamicVertexBuffer.h"
#include "FrameContext.h"
#include "GlobalHeap.h"
#include "Library.h"
#include "Primitive.h"
#include "SingleTimeCommandBuffer.h"
#include "StructuredBuffer.h"

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

struct Skin {
  StructuredBuffer<uint32_t> jointMap;
};

struct Node {
  glm::mat4 inverseBindPose = glm::mat4(1.0f);
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
      GlobalHeap& heap,
      const std::string& path,
      const glm::mat4& transform = glm::mat4(1.0f));

  void setNodeRelativeTransform(uint32_t nodeIdx, const glm::mat4& transform);
  void recomputeTransforms();
  void uploadTransforms(const FrameContext& frame) {
    _nodeTransforms.upload(frame.frameRingBufferIndex);
  }
  const DynamicVertexBuffer<glm::mat4>& getTransformsBuffer() const {
    return _nodeTransforms;
  }
  BufferHandle getTransformsHandle(const FrameContext& frame) const {
    return _nodeTransforms.getCurrentBufferHandle(frame.frameRingBufferIndex);
  }
  void setModelTransform(const glm::mat4& modelTransform);
  BufferHandle getSkinJointMapHandle(uint32_t skinIdx) {
    return _skins[skinIdx].jointMap.getHandle();
  }
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

  const CesiumGltf::Model& getGltfModel() const { return _model; }

  const std::vector<Primitive>& getPrimitives() const {
    return this->_primitives;
  }

  std::vector<Primitive>& getPrimitives() { return this->_primitives; }

private:
  CesiumGltf::Model _model;
  std::vector<Node> _nodes;
  std::vector<Mesh> _meshes;

  std::vector<Skin> _skins;
  DynamicVertexBuffer<glm::mat4> _nodeTransforms;
  std::vector<Primitive> _primitives;
  std::vector<Material> _materials;
  std::vector<Texture> _textures;

  glm::mat4 _modelTransform;

  void _updateTransforms(int32_t nodeIdx, const glm::mat4& transform);
  void _loadNode(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap,
      int32_t nodeIdx,
      const glm::mat4& parentTransform);
};
} // namespace AltheaEngine
