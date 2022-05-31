#include "Model.h"
#include "FileAssetAccessor.h"
#include "TaskProcessor.h"

#include "Utilities.h"
#include <gsl/span>

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGltfReader/GltfReader.h>
#include <memory>

#include <vulkan/vulkan.h>

Model::Model(
    const VkDevice& device, 
    const VkPhysicalDevice& physicalDevice, 
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

  // TODO: create primitives properly
  for (const CesiumGltf::MeshPrimitive& primitive : this->_model.meshes[0].primitives) {
    this->_primitives.push_back(
        Primitive(
          device, 
          physicalDevice, 
          this->_model, 
          primitive));
  }
}

void Model::render(const VkCommandBuffer& commandBuffer) const {
  for (const Primitive& primitive : this->_primitives) {
    primitive.render(commandBuffer);
    if (1) {
      return;
    }
  }
}

void Model::destroy(const VkDevice& device) {
  for (Primitive& primitive : this->_primitives) {
    primitive.destroy(device);
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
    const VkDevice& device,  
    const VkPhysicalDevice& physicalDevice,
    const std::string& graphicsPipelineName,
    const ConfigParser& configParser) {      
  ModelsConfigCategory modelsList(graphicsPipelineName);
  configParser.parseCategory(modelsList);

  this->_models.reserve(modelsList.modelNames.size());
  for (const std::string& modelName : modelsList.modelNames) {
    this->_models.push_back(Model(device, physicalDevice, modelName));
  }
}

void ModelManager::render(const VkCommandBuffer& commandBuffer) const {
  for (const Model& model : this->_models) {
    model.render(commandBuffer);
  }
}

void ModelManager::destroy(const VkDevice& device) {
  for (Model& model : this->_models) {
    model.destroy(device);
  }
}