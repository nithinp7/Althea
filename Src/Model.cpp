#include "Model.h"
#include "Application.h"
#include "FileAssetAccessor.h"
#include "TaskProcessor.h"

#include "Utilities.h"
#include <gsl/span>

#include <glm/gtc/quaternion.hpp>

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGltfReader/GltfReader.h>
#include <memory>
#include <array>

#include <vulkan/vulkan.h>

Model::Model(
    const Application& app, 
    const std::string& path) {
  // TODO: just for testing
  static CesiumAsync::AsyncSystem async(std::make_shared<TaskProcessor>());

  std::vector<char> modelFile = Utilities::readFile(path);

  CesiumGltfReader::GltfReaderOptions options;
  // TODO:
  // options.ktx2TranscodeTargets ...
  CesiumGltfReader::GltfReader reader;
  CesiumGltfReader::GltfReaderResult result = 
      reader.readGltf(
        gsl::span<const std::byte>(
          reinterpret_cast<const std::byte*>(modelFile.data()), 
          modelFile.size()),
        options);
  
  CesiumAsync::Future<CesiumGltfReader::GltfReaderResult> futureResult = 
      reader.resolveExternalData(
        async,
        // this might not be a valid base url, may need to remove file name
        path,
        {},
        std::make_shared<FileAssetAccessor>(),
        options,
        std::move(result));

  // All on main thread right now, but not safe in general.
  result = futureResult.wait();
  
  if (!result.model) {
    return;
  }

  // Not really movable
  this->_model = std::move(*result.model);
  this->_model.generateMissingNormalsSmooth();

  glm::mat4 transform(1.0f);

  if (this->_model.scene >= 0 && this->_model.scene < this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[this->_model.scene];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_loadNode(app, this->_model, this->_model.nodes[nodeId], transform);
      }
    }
  } else if (this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[0];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_loadNode(app, this->_model, this->_model.nodes[nodeId], transform);
      }
    }
  } else if (this->_model.nodes.size()) {
    this->_loadNode(app, this->_model, this->_model.nodes[0], transform);
  } else {
    for (const CesiumGltf::Mesh& mesh : this->_model.meshes) {
      for (const CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
        this->_primitives.emplace_back(
            app, 
            this->_model, 
            primitive,
            transform);
      }
    }
  }
}

void Model::updateUniforms(
    const glm::mat4& view, const glm::mat4& projection, uint32_t currentFrame) const {
  for (const Primitive& primitive : this->_primitives) {
    primitive.updateUniforms(glm::mat4(1.0f), view, projection, currentFrame);
  }
}

size_t Model::getPrimitivesCount() const {
  return this->_primitives.size();
}

void Model::assignDescriptorSets(std::vector<VkDescriptorSet>& availableDescriptorSets) {
  for (Primitive& primitive : this->_primitives) {
    primitive.assignDescriptorSets(availableDescriptorSets);
  }
}

void Model::render(
    const VkCommandBuffer& commandBuffer, 
    const VkPipelineLayout& pipelineLayout,
    uint32_t currentFrame) const {
  for (const Primitive& primitive : this->_primitives) {
    primitive.render(commandBuffer, pipelineLayout, currentFrame);
  }
}

void Model::_loadNode(
    const Application& app,
    const CesiumGltf::Model& model, 
    const CesiumGltf::Node& node, 
    const glm::mat4& transform) {
  static constexpr std::array<double, 16> identityMatrix = {
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0};

  glm::mat4 nodeTransform = transform;

  const std::vector<double>& matrix = node.matrix;
  bool isIdentityMatrix = false;
  if (matrix.size() == 16) {
    isIdentityMatrix =
        std::equal(matrix.begin(), matrix.end(), identityMatrix.begin());
  }

  if (matrix.size() == 16 && !isIdentityMatrix) {
    glm::mat4x4 nodeTransformGltf(
        glm::vec4(static_cast<float>(matrix[0]), static_cast<float>(matrix[1]), static_cast<float>(matrix[2]), static_cast<float>(matrix[3])),
        glm::vec4(static_cast<float>(matrix[4]), static_cast<float>(matrix[5]), static_cast<float>(matrix[6]), static_cast<float>(matrix[7])),
        glm::vec4(static_cast<float>(matrix[8]), static_cast<float>(matrix[9]), static_cast<float>(matrix[10]), static_cast<float>(matrix[11])),
        glm::vec4(static_cast<float>(matrix[12]), static_cast<float>(matrix[13]), static_cast<float>(matrix[14]), static_cast<float>(matrix[15])));

    nodeTransform = nodeTransform * nodeTransformGltf;
  } else {
    glm::mat4 translation(1.0);
    if (node.translation.size() == 3) {
      translation[3] = glm::vec4(
          static_cast<float>(node.translation[0]),
          static_cast<float>(node.translation[1]),
          static_cast<float>(node.translation[2]),
          1.0);
    }

    glm::quat rotationQuat(1.0, 0.0, 0.0, 0.0);
    if (node.rotation.size() == 4) {
      rotationQuat[0] = static_cast<float>(node.rotation[0]);
      rotationQuat[1] = static_cast<float>(node.rotation[1]);
      rotationQuat[2] = static_cast<float>(node.rotation[2]);
      rotationQuat[3] = static_cast<float>(node.rotation[3]);
    }

    glm::mat4 scale(1.0);
    if (node.scale.size() == 3) {
      scale[0].x = static_cast<float>(node.scale[0]);
      scale[1].y = static_cast<float>(node.scale[1]);
      scale[2].z = static_cast<float>(node.scale[2]);
    }

    nodeTransform =
        nodeTransform * translation * glm::mat4(rotationQuat) * scale;
  }

  if (node.mesh >= 0 && node.mesh < model.meshes.size()) {
    const CesiumGltf::Mesh& mesh = model.meshes[node.mesh];
    for (const CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
      this->_primitives.emplace_back(
        app, 
        this->_model, 
        primitive,
        nodeTransform);
    }
  }

  for (int32_t childNodeId : node.children) {
    if (childNodeId >= 0 && childNodeId < model.nodes.size()) {
      this->_loadNode(app, model, model.nodes[childNodeId], nodeTransform);
    }
  }
}

namespace {
class ModelsConfigCategory : public ConfigCategory {
private: 
  std::string _graphicsPipelineName;
public:
  std::vector<std::string> modelNames;

  ModelsConfigCategory(const std::string& graphicsPipelineName) 
    : ConfigCategory("MODELS_LIST"),
      _graphicsPipelineName(graphicsPipelineName) {}

  void parseLine(const std::string& line) override {
    size_t split = line.find(" ");
    if (split != std::string::npos && 
        line.substr(0, split) == this->_graphicsPipelineName) {
      this->modelNames.push_back(
          "../Content/Models/" + line.substr(split + 1, line.size()));
    }
  }
};
} // namespace

ModelManager::ModelManager(
    const Application& app,
    const std::string& graphicsPipelineName) {      
  ModelsConfigCategory modelsList(graphicsPipelineName);
  app.getConfigParser().parseCategory(modelsList);

  this->_models.reserve(modelsList.modelNames.size());
  for (const std::string& modelName : modelsList.modelNames) {
    this->_models.emplace_back(app, modelName);
  }
}

void ModelManager::updateUniforms(
    const glm::mat4& view, const glm::mat4& projection, uint32_t currentFrame) const {
  for (const Model& model : this->_models) {
    model.updateUniforms(view, projection, currentFrame);
  }
}

size_t ModelManager::getPrimitivesCount() const {
  size_t primitivesCount = 0;
  for (const Model& model : this->_models) {
    primitivesCount += model.getPrimitivesCount();
  }
  return primitivesCount;
}

void ModelManager::assignDescriptorSets(std::vector<VkDescriptorSet>& availableDescriptorSets) {
  for (Model& model : this->_models) {
    model.assignDescriptorSets(availableDescriptorSets);
  }
}

void ModelManager::render(
    const VkCommandBuffer& commandBuffer,
    const VkPipelineLayout& pipelineLayout, 
    uint32_t currentFrame) const {
  for (const Model& model : this->_models) {
    model.render(commandBuffer, pipelineLayout, currentFrame);
  }
}