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
    const std::string& path,
    DescriptorSetAllocator* materialAllocator) {
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

  // TODO: Create only one primitive for each mesh and position / instance
  // it as dictated by nodes.

  // Sensible estimate of number of primitives that will be in the model
  this->_primitives.reserve(this->_model.meshes.size());
  if (this->_model.scene >= 0 &&
      this->_model.scene < this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[this->_model.scene];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_loadNode(
            app,
            commandBuffer,
            nodeId,
            _modelTransform,
            materialAllocator);
      }
    }
  } else if (this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[0];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_loadNode(
            app,
            commandBuffer,
            nodeId,
            _modelTransform,
            materialAllocator);
      }
    }
  } else if (this->_model.nodes.size()) {
    this->_loadNode(app, commandBuffer, 0, _modelTransform, materialAllocator);
  } else {
    for (const CesiumGltf::Mesh& mesh : this->_model.meshes) {
      for (const CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
        this->_primitives.emplace_back(
            app,
            commandBuffer,
            this->_model,
            primitive,
            _modelTransform,
            materialAllocator);
      }
    }
  }
}

void Model::registerToHeap(GlobalHeap& heap) {
  for (Primitive& primitive : this->_primitives) {
    primitive.registerToHeap(heap);
  }
}

void Model::createConstantBuffers(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap) {
  for (Primitive& primitive : this->_primitives) {
    primitive.createConstantBuffer(app, commandBuffer, heap);
  }
}

void Model::setModelTransform(const glm::mat4& modelTransform) {
  _modelTransform = modelTransform;

  if (this->_model.scene >= 0 &&
      this->_model.scene < this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[this->_model.scene];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_updateTransforms(nodeId, modelTransform);
      }
    }
  } else if (this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[0];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        this->_updateTransforms(nodeId, modelTransform);
      }
    }
  } else if (this->_model.nodes.size()) {
    this->_updateTransforms(0, modelTransform);
  } else {
    for (const Mesh& mesh : _meshes) {
      for (int32_t primIdx = mesh.primitiveStartIdx;
           primIdx < (mesh.primitiveStartIdx + mesh.primitiveCount);
           ++primIdx) {
        this->_primitives[primIdx].setTransform(modelTransform);
      }
    }
  }
}

size_t Model::getPrimitivesCount() const { return this->_primitives.size(); }

void Model::draw(const DrawContext& context) const {
  if (this->_model.scene >= 0 &&
      this->_model.scene < this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[this->_model.scene];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        _drawNode(context, nodeId);
      }
    }
  } else if (this->_model.scenes.size()) {
    const CesiumGltf::Scene& scene = this->_model.scenes[0];
    for (int32_t nodeId : scene.nodes) {
      if (nodeId >= 0 && nodeId < this->_model.nodes.size()) {
        _drawNode(context, nodeId);
      }
    }
  } else if (this->_model.nodes.size()) {
    _drawNode(context, 0);
  } else {
    for (const Mesh& mesh : _meshes) {
      for (int32_t primIdx = mesh.primitiveStartIdx;
           primIdx < (mesh.primitiveStartIdx + mesh.primitiveCount);
           ++primIdx) {
        _primitives[primIdx].draw(context);
      }
    }
  }
}

void Model::_updateTransforms(
    int32_t nodeIdx,
    const glm::mat4& parentTransform) {
  // TODO: previous transforms also needed for velocity buffer
  const Node& node = _nodes[nodeIdx];
  glm::mat4 transform = parentTransform * _nodes[nodeIdx].currentTransform;

  if (node.meshIdx >= 0) {
    const Mesh& mesh = _meshes[node.meshIdx];
    for (uint32_t primIdx = mesh.primitiveStartIdx;
         primIdx < (mesh.primitiveStartIdx + mesh.primitiveCount);
         ++primIdx) {
      _primitives[primIdx].setTransform(transform);
    }
  }

  for (int32_t childNodeId : _model.nodes[nodeIdx].children) {
    if (childNodeId >= 0 && childNodeId < _nodes.size()) {
      _updateTransforms(childNodeId, transform);
    }
  }
}

void Model::_drawNode(const DrawContext& context, int32_t nodeIdx) const {
  // TODO: previous transforms also needed for velocity buffer
  const Node& node = _nodes[nodeIdx];

  if (node.meshIdx >= 0) {
    const Mesh& mesh = _meshes[node.meshIdx];
    for (uint32_t primIdx = mesh.primitiveStartIdx;
         primIdx < (mesh.primitiveStartIdx + mesh.primitiveCount);
         ++primIdx) {
      _primitives[primIdx].draw(context);
    }
  }

  for (int32_t childNodeId : _model.nodes[nodeIdx].children) {
    if (childNodeId >= 0 && childNodeId < _nodes.size()) {
      this->_drawNode(context, childNodeId);
    }
  }
}

void Model::_loadNode(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    int32_t nodeIdx,
    const glm::mat4& parentTransform,
    DescriptorSetAllocator* materialAllocator) {
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
          this->_model,
          primitive,
          nodeTransform,
          materialAllocator);
    }
  }

  for (int32_t childNodeId : gltfNode.children) {
    if (childNodeId >= 0 && childNodeId < _model.nodes.size()) {
      this->_loadNode(
          app,
          commandBuffer,
          childNodeId,
          nodeTransform,
          materialAllocator);
    }
  }
}

void Model::updateAnimation(float deltaTime) {
  if (_activeAnimation < 0)
    return;

  struct NodeTransform {
    glm::quat rotation{};
    glm::vec3 translation{};
    glm::vec3 scale = glm::vec3(1.0f);
    bool targeted = false;
  };
  ALTHEA_STACK_VECTOR(nodeTransforms, NodeTransform, _nodes.size());
  nodeTransforms.resize(_nodes.size());
  for (uint32_t i = 0; i < _nodes.size(); ++i) {
    NodeTransform& transform = nodeTransforms[i];
    const CesiumGltf::Node& gltfNode = _model.nodes[i];
    glm::mat4 translation(1.0);
    if (gltfNode.translation.size() == 3) {
      transform.translation = glm::vec3(
          static_cast<float>(gltfNode.translation[0]),
          static_cast<float>(gltfNode.translation[1]),
          static_cast<float>(gltfNode.translation[2]));
    }

    if (gltfNode.rotation.size() == 4) {
      transform.rotation[0] = static_cast<float>(gltfNode.rotation[0]);
      transform.rotation[1] = static_cast<float>(gltfNode.rotation[1]);
      transform.rotation[2] = static_cast<float>(gltfNode.rotation[2]);
      transform.rotation[3] = static_cast<float>(gltfNode.rotation[3]);
    }

    if (gltfNode.scale.size() == 3) {
      transform.scale[0] = static_cast<float>(gltfNode.scale[0]);
      transform.scale[1] = static_cast<float>(gltfNode.scale[1]);
      transform.scale[2] = static_cast<float>(gltfNode.scale[2]);
    }
  }

  const CesiumGltf::Animation& animation = _model.animations[_activeAnimation];
  for (const CesiumGltf::AnimationChannel& channel : animation.channels) {
    const CesiumGltf::AnimationSampler& sampler =
        animation.samplers[channel.sampler];
    CesiumGltf::AccessorView<float> timeSamples(_model, sampler.input);

    if (_animationLooping &&
        _animationTime > timeSamples[timeSamples.size() - 1]) {
      _animationTime -= timeSamples[timeSamples.size() - 1];
    }

    // TODO: track last used sample to avoid linear scan?
    // TODO: implement other interpolators...
    uint32_t sampleIdx = 0;
    float u = 0.0f;
    for (; sampleIdx < timeSamples.size() - 1; ++sampleIdx) {
      float t0 = timeSamples[sampleIdx];
      float t1 = timeSamples[sampleIdx + 1];
      if (_animationTime >= t0 && _animationTime <= t1) {
        u = (_animationTime - t0) / (t1 - t0);
        break;
      }
    }

    if (sampleIdx == timeSamples.size() - 1)
      continue;

    NodeTransform& transform = nodeTransforms[channel.target.node];
    transform.targeted = true;
    if (channel.target.path ==
        CesiumGltf::AnimationChannelTarget::Path::translation) {
      CesiumGltf::AccessorView<glm::vec3> translations(_model, sampler.output);
      transform.translation =
          glm::mix(translations[sampleIdx], translations[sampleIdx + 1], u);
    } else if (
        channel.target.path ==
        CesiumGltf::AnimationChannelTarget::Path::rotation) {
      CesiumGltf::AccessorView<glm::quat> rotations(_model, sampler.output);
      transform.rotation =
          glm::slerp(rotations[sampleIdx], rotations[sampleIdx + 1], u);
    } else if (
        channel.target.path ==
        CesiumGltf::AnimationChannelTarget::Path::scale) {
      CesiumGltf::AccessorView<glm::vec3> scales(_model, sampler.output);
      transform.scale = glm::mix(scales[sampleIdx], scales[sampleIdx + 1], u);
    } else if (
        channel.target.path ==
        CesiumGltf::AnimationChannelTarget::Path::weights) {
      // TODO:
    }
  }

  for (uint32_t i = 0; i < _nodes.size(); ++i) {
    Node& node = _nodes[i];
    const NodeTransform& transform = nodeTransforms[i];
    if (transform.targeted) {
      glm::mat4 relativeTransform(transform.rotation);
      relativeTransform[0] *= transform.scale.x;
      relativeTransform[1] *= transform.scale.y;
      relativeTransform[2] *= transform.scale.z;
      relativeTransform[3] = glm::vec4(transform.translation, 1.0f);

      node.currentTransform = relativeTransform;
    }
  }

  setModelTransform(_modelTransform);

  _animationTime += deltaTime;
}
void Model::startAnimation(int32_t idx, bool bLoop) {
  _activeAnimation = idx;
  _animationTime = 0.0f;
  _animationLooping = bLoop;
}
} // namespace AltheaEngine
