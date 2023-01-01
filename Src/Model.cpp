#include "Model.h"
#include "Application.h"
#include "FileAssetAccessor.h"
#include "TaskProcessor.h"
#include "GraphicsPipeline.h"
#include "DescriptorSet.h"

#include "Utilities.h"
#include <gsl/span>

#include <glm/gtc/quaternion.hpp>

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGltfReader/GltfReader.h>
#include <memory>
#include <array>

#include <filesystem>

#include <vulkan/vulkan.h>

namespace AltheaEngine {
static CesiumAsync::Future<CesiumGltfReader::GltfReaderResult> resolveExternalData(
    CesiumAsync::AsyncSystem asyncSystem,
    const std::string& baseUrl,
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor,
    const CesiumGltfReader::GltfReaderOptions& options,
    CesiumGltfReader::GltfReaderResult&& result) {

  if (!result.model) {
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  // Get a rough count of how many external buffers we may have.
  // Some of these may be data uris though.
  size_t uriBuffersCount = 0;
  for (const CesiumGltf::Buffer& buffer : result.model->buffers) {
    if (buffer.uri) {
      ++uriBuffersCount;
    }
  }

  for (const CesiumGltf::Image& image : result.model->images) {
    if (image.uri) {
      ++uriBuffersCount;
    }
  }

  if (uriBuffersCount == 0) {
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  auto pResult = std::make_unique<CesiumGltfReader::GltfReaderResult>(std::move(result));

  struct ExternalBufferLoadResult {
    bool success = false;
    std::string bufferUri;
  };

  std::vector<CesiumAsync::Future<ExternalBufferLoadResult>> resolvedBuffers;
  resolvedBuffers.reserve(uriBuffersCount);

  // We need to skip data uris.
  constexpr std::string_view dataPrefix = "data:";
  constexpr size_t dataPrefixLength = dataPrefix.size();

  std::filesystem::path basePath(baseUrl);

  for (CesiumGltf::Buffer& buffer : pResult->model->buffers) {
    if (buffer.uri && buffer.uri->substr(0, dataPrefixLength) != dataPrefix) {
      resolvedBuffers.push_back(
          pAssetAccessor
              ->get(asyncSystem, basePath.parent_path().append(*buffer.uri).string(), {})
              .thenInWorkerThread(
                  [pBuffer =
                   &buffer](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                    const CesiumAsync::IAssetResponse* pResponse = pRequest->response();

                    std::string bufferUri = *pBuffer->uri;

                    if (pResponse) {
                      pBuffer->uri = std::nullopt;
                      pBuffer->cesium.data = std::vector<std::byte>(
                          pResponse->data().begin(),
                          pResponse->data().end());
                      return ExternalBufferLoadResult{true, bufferUri};
                    }

                    return ExternalBufferLoadResult{false, bufferUri};
                  }));
    }
  }

  for (CesiumGltf::Image& image : pResult->model->images) {
    if (image.uri && image.uri->substr(0, dataPrefixLength) != dataPrefix) {
      resolvedBuffers.push_back(
          pAssetAccessor
              ->get(asyncSystem, basePath.parent_path().append(*image.uri).string(), {})
              .thenInWorkerThread(
                  [pImage = &image,
                   ktx2TranscodeTargets = options.ktx2TranscodeTargets](
                      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                    const CesiumAsync::IAssetResponse* pResponse = pRequest->response();

                    std::string imageUri = *pImage->uri;

                    if (pResponse) {
                      pImage->uri = std::nullopt;

                      CesiumGltfReader::ImageReaderResult imageResult =
                          CesiumGltfReader::GltfReader::readImage(pResponse->data(), ktx2TranscodeTargets);
                      if (imageResult.image) {
                        pImage->cesium = std::move(*imageResult.image);
                        return ExternalBufferLoadResult{true, imageUri};
                      }
                    }

                    return ExternalBufferLoadResult{false, imageUri};
                  }));
    }
  }

  return asyncSystem.all(std::move(resolvedBuffers))
      .thenInWorkerThread(
          [pResult = std::move(pResult)](
              std::vector<ExternalBufferLoadResult>&& loadResults) mutable {
            for (auto& bufferResult : loadResults) {
              if (!bufferResult.success) {
                pResult->warnings.push_back(
                    "Could not load the external gltf buffer: " +
                    bufferResult.bufferUri);
              }
            }
            return std::move(*pResult.release());
          });
}

Model::Model(
    const Application& app, 
    const std::string& name,
    DescriptorSetAllocator& materialAllocator) {
  std::string path = "../Content/Models/" + name;

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
      resolveExternalData(
        async,
        // this might not be a valid base url, may need to remove file name
        path,
        std::make_shared<FileAssetAccessor>(),
        options,
        std::move(result));

  // All on main thread right now, but not safe in general.
  result = futureResult.wait();
  
  if (!result.model) {
    return;
  }

  this->_model = std::move(*result.model);
  // this->_model.generateMissingNormalsSmooth();

  glm::mat4 transform(1.0f);

  if (this->_model.scene >= 0 && this->_model.scene < this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[this->_model.scene];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_loadNode(app, this->_model, this->_model.nodes[nodeId], transform, materialAllocator);
      }
    }
  } else if (this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[0];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_loadNode(app, this->_model, this->_model.nodes[nodeId], transform, materialAllocator);
      }
    }
  } else if (this->_model.nodes.size()) {
    this->_loadNode(app, this->_model, this->_model.nodes[0], transform, materialAllocator);
  } else {
    for (const CesiumGltf::Mesh& mesh : this->_model.meshes) {
      for (const CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
        this->_primitives.push_back(std::make_unique<Primitive>(
            app, 
            this->_model, 
            primitive,
            transform,
            materialAllocator));
      }
    }
  }
}

size_t Model::getPrimitivesCount() const {
  return this->_primitives.size();
}

void Model::updateUniforms(
    const glm::mat4& view, const glm::mat4& projection, const FrameContext& frame) const {
  for (const std::unique_ptr<Primitive>& pPrimitive : this->_primitives) {
    pPrimitive->updateUniforms(glm::mat4(1.0f), view, projection, frame.frameRingBufferIndex);
  }
}

void Model::draw(
    const VkCommandBuffer& commandBuffer, 
    const VkPipelineLayout& pipelineLayout,
    const FrameContext& frame) const {
  for (const std::unique_ptr<Primitive>& pPrimitive : 
       this->_primitives) {
    pPrimitive->draw(commandBuffer, pipelineLayout, frame);
  }
}

void Model::_loadNode(
    const Application& app,
    const CesiumGltf::Model& model, 
    const CesiumGltf::Node& node, 
    const glm::mat4& transform,
    DescriptorSetAllocator& materialAllocator) {
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
      this->_primitives.push_back(std::make_unique<Primitive>(
        app, 
        this->_model, 
        primitive,
        nodeTransform,
        materialAllocator));
    }
  }

  for (int32_t childNodeId : node.children) {
    if (childNodeId >= 0 && childNodeId < model.nodes.size()) {
      this->_loadNode(app, model, model.nodes[childNodeId], nodeTransform, materialAllocator);
    }
  }
}
} // namespace AltheaEngine
