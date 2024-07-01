#include "Model.h"

#include "Application.h"
#include "Containers/StackVector.h"
#include "DescriptorSet.h"
#include "FileAssetAccessor.h"
#include "GraphicsPipeline.h"
#include "TaskProcessor.h"
#include "Utilities.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltfReader/GltfReader.h>
#include <glm/gtc/quaternion.hpp>
#include <gsl/span>
#include <vulkan/vulkan.h>

#include <array>
#include <filesystem>
#include <memory>

namespace AltheaEngine {
static CesiumAsync::Future<CesiumGltfReader::GltfReaderResult>
resolveExternalData(
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

  auto pResult =
      std::make_unique<CesiumGltfReader::GltfReaderResult>(std::move(result));

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
              ->get(
                  asyncSystem,
                  basePath.parent_path().append(*buffer.uri).string(),
                  {})
              .thenInWorkerThread(
                  [pBuffer = &buffer](
                      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                    const CesiumAsync::IAssetResponse* pResponse =
                        pRequest->response();

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
              ->get(
                  asyncSystem,
                  basePath.parent_path().append(*image.uri).string(),
                  {})
              .thenInWorkerThread(
                  [pImage = &image,
                   ktx2TranscodeTargets = options.ktx2TranscodeTargets](
                      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                    const CesiumAsync::IAssetResponse* pResponse =
                        pRequest->response();

                    std::string imageUri = *pImage->uri;

                    if (pResponse) {
                      pImage->uri = std::nullopt;

                      CesiumGltfReader::ImageReaderResult imageResult =
                          CesiumGltfReader::GltfReader::readImage(
                              pResponse->data(),
                              ktx2TranscodeTargets);
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
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap,
    const std::string& path) {
  // TODO: just for testing
  static CesiumAsync::AsyncSystem async(std::make_shared<TaskProcessor>());

  std::vector<char> modelFile = Utilities::readFile(path);

  CesiumGltfReader::GltfReaderOptions options;
  // TODO:
  // options.ktx2TranscodeTargets ...
  CesiumGltfReader::GltfReader reader;
  CesiumGltfReader::GltfReaderResult result = reader.readGltf(
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

  // Generate mip-maps for all images
  for (CesiumGltf::Image& image : this->_model.images) {
    if (CesiumGltfReader::GltfReader::generateMipMaps(image.cesium)) {
      throw std::runtime_error(
          "Could not generate mip-maps for images in glTF.");
    }
  }

  _modelTransform = glm::mat4(1.0f);
  _nodes.resize(_model.nodes.size());

  for (const CesiumGltf::Skin& gltfSkin : _model.skins) {
    // joint maps are used to lookup node indices in order to fetch
    // transforms
    Skin& skin = _skins.emplace_back();
    skin.jointMap = StructuredBuffer<uint32_t>(app, gltfSkin.joints.size());
    for (uint32_t jointIdx = 0; jointIdx < gltfSkin.joints.size(); ++jointIdx) {
      skin.jointMap.setElement(gltfSkin.joints[jointIdx], jointIdx);
    }
    skin.jointMap.upload(app, commandBuffer);
    skin.jointMap.registerToHeap(heap);

    // setup inverse bind matrices if applicable
    if (gltfSkin.inverseBindMatrices >= 0) {
      CesiumGltf::AccessorView<glm::mat4> inverseBindMatrices(
          _model,
          gltfSkin.inverseBindMatrices);
      for (uint32_t jointIdx = 0; jointIdx < gltfSkin.joints.size();
           ++jointIdx) {
        _nodes[gltfSkin.joints[jointIdx]].inverseBindPose =
            inverseBindMatrices[jointIdx];
      }
    }
  }

  // Sensible estimate of number of primitives that will be in the model
  this->_primitives.reserve(this->_model.meshes.size());
  if (this->_model.scene >= 0 &&
      this->_model.scene < this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[this->_model.scene];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_loadNode(app, commandBuffer, heap, nodeId, _modelTransform);
      }
    }
  } else if (this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[0];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_loadNode(app, commandBuffer, heap, nodeId, _modelTransform);
      }
    }
  } else if (this->_model.nodes.size()) {
    this->_loadNode(app, commandBuffer, heap, 0, _modelTransform);
  } else {
    for (const CesiumGltf::Mesh& mesh : this->_model.meshes) {
      for (const CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
        this->_primitives.emplace_back(
            app,
            commandBuffer,
            heap,
            this->_model,
            primitive,
            BufferHandle(),
            0);
      }
    }
  }

  // init transforms buffer
  _nodeTransforms =
      DynamicVertexBuffer<glm::mat4>(app, glm::max(_nodes.size(), 1ull), true);
  _nodeTransforms.registerToHeap(heap);
  recomputeTransforms();
  for (uint32_t ringBufferIdx = 0; ringBufferIdx < MAX_FRAMES_IN_FLIGHT;
       ++ringBufferIdx) {
    _nodeTransforms.upload(ringBufferIdx);
  }
}

void Model::recomputeTransforms() {
  if (this->_model.scene >= 0 &&
      this->_model.scene < this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[this->_model.scene];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_updateTransforms(nodeId, _modelTransform);
      }
    }
  } else if (this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[0];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_updateTransforms(nodeId, _modelTransform);
      }
    }
  } else if (this->_model.nodes.size()) {
    this->_updateTransforms(0, _modelTransform);
  }

  if (_nodes.size() == 0) {
    _nodeTransforms.setVertex(_modelTransform, 0);
  }
}

void Model::setModelTransform(const glm::mat4& modelTransform) {
  _modelTransform = modelTransform;
}

void Model::setNodeRelativeTransform(
    uint32_t nodeIdx,
    const glm::mat4& transform) {
  _nodes[nodeIdx].currentTransform = transform;
}

size_t Model::getPrimitivesCount() const { return this->_primitives.size(); }

void Model::_updateTransforms(
    int32_t nodeIdx,
    const glm::mat4& parentTransform) {
  // TODO: previous transforms also needed for velocity buffer
  const Node& node = _nodes[nodeIdx];
  glm::mat4 transform = parentTransform * _nodes[nodeIdx].currentTransform;

  _nodeTransforms.setVertex(transform * node.inverseBindPose, nodeIdx);

  for (int32_t childNodeId : _model.nodes[nodeIdx].children) {
    if (childNodeId >= 0 && childNodeId < _nodes.size()) {
      _updateTransforms(childNodeId, transform);
    }
  }
}

void Model::_loadNode(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap,
    int32_t nodeIdx,
    const glm::mat4& parentTransform) {
  const CesiumGltf::Node& gltfNode = _model.nodes[nodeIdx];
  Node& node = _nodes[nodeIdx];

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

  const std::vector<double>& matrix = gltfNode.matrix;
  bool isIdentityMatrix = false;
  if (matrix.size() == 16) {
    isIdentityMatrix =
        std::equal(matrix.begin(), matrix.end(), identityMatrix.begin());
  }

  if (matrix.size() == 16 && !isIdentityMatrix) {
    node.currentTransform = glm::mat4(
        glm::vec4(
            static_cast<float>(matrix[0]),
            static_cast<float>(matrix[1]),
            static_cast<float>(matrix[2]),
            static_cast<float>(matrix[3])),
        glm::vec4(
            static_cast<float>(matrix[4]),
            static_cast<float>(matrix[5]),
            static_cast<float>(matrix[6]),
            static_cast<float>(matrix[7])),
        glm::vec4(
            static_cast<float>(matrix[8]),
            static_cast<float>(matrix[9]),
            static_cast<float>(matrix[10]),
            static_cast<float>(matrix[11])),
        glm::vec4(
            static_cast<float>(matrix[12]),
            static_cast<float>(matrix[13]),
            static_cast<float>(matrix[14]),
            static_cast<float>(matrix[15])));
  } else {
    glm::mat4 translation(1.0);
    if (gltfNode.translation.size() == 3) {
      translation[3] = glm::vec4(
          static_cast<float>(gltfNode.translation[0]),
          static_cast<float>(gltfNode.translation[1]),
          static_cast<float>(gltfNode.translation[2]),
          1.0);
    }

    glm::quat rotationQuat(1.0, 0.0, 0.0, 0.0);
    if (gltfNode.rotation.size() == 4) {
      rotationQuat[0] = static_cast<float>(gltfNode.rotation[0]);
      rotationQuat[1] = static_cast<float>(gltfNode.rotation[1]);
      rotationQuat[2] = static_cast<float>(gltfNode.rotation[2]);
      rotationQuat[3] = static_cast<float>(gltfNode.rotation[3]);
    }

    glm::mat4 scale(1.0);
    if (gltfNode.scale.size() == 3) {
      scale[0].x = static_cast<float>(gltfNode.scale[0]);
      scale[1].y = static_cast<float>(gltfNode.scale[1]);
      scale[2].z = static_cast<float>(gltfNode.scale[2]);
    }

    node.currentTransform = translation * glm::mat4(rotationQuat) * scale;
  }

  glm::mat4 nodeTransform = parentTransform * node.currentTransform;

  if (gltfNode.mesh >= 0 && gltfNode.mesh < _model.meshes.size()) {
    const CesiumGltf::Mesh& gltfMesh = _model.meshes[gltfNode.mesh];
    node.meshIdx = _meshes.size();
    Mesh& mesh = _meshes.emplace_back();
    mesh.primitiveStartIdx = this->_primitives.size();
    mesh.primitiveCount = gltfMesh.primitives.size();
    for (const CesiumGltf::MeshPrimitive& primitive : gltfMesh.primitives) {
      this->_primitives.emplace_back(
          app,
          commandBuffer,
          heap,
          this->_model,
          primitive,
          gltfNode.skin >= 0 ? getSkinJointMapHandle(gltfNode.skin)
                             : BufferHandle(),
          nodeIdx);
    }
  }

  for (int32_t childNodeId : gltfNode.children) {
    if (childNodeId >= 0 && childNodeId < _model.nodes.size()) {
      this->_loadNode(app, commandBuffer, heap, childNodeId, nodeTransform);
    }
  }
}
} // namespace AltheaEngine
