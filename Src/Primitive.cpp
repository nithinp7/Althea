#include "Primitive.h"

#include "Application.h"
#include "DefaultTextures.h"
#include "DescriptorSet.h"
#include "GeometryUtilities.h"
#include "GraphicsPipeline.h"
#include "ModelViewProjection.h"
#include "Utilities.h"

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/TextureInfo.h>
#include <glm/gtc/matrix_transform.hpp>

#include <string>

namespace AltheaEngine {
namespace {
struct PrimitivePushConstants {
  uint32_t constantBufferHandle{};
  uint32_t matrixBufferHandle{};
};
} // namespace
static std::shared_ptr<Texture> createTexture(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const CesiumGltf::Model& model,
    const std::optional<CesiumGltf::TextureInfo>& texture,
    std::unordered_map<const CesiumGltf::Texture*, std::shared_ptr<Texture>>&
        textureMap,
    int32_t& textureCoordinateIndexConstant,
    bool srgb) {
  if (texture && texture->index >= 0 &&
      texture->index <= model.textures.size() &&
      static_cast<int32_t>(texture->texCoord) < MAX_UV_COORDS) {
    textureCoordinateIndexConstant = static_cast<int32_t>(texture->texCoord);

    const CesiumGltf::Texture& gltfTexture = model.textures[texture->index];
    auto textureIt = textureMap.find(&gltfTexture);
    if (textureIt != textureMap.end()) {
      return textureIt->second;
    }

    auto result = textureMap.emplace(
        &gltfTexture,
        std::make_shared<Texture>(
            app,
            commandBuffer,
            model,
            model.textures[texture->index],
            srgb));
    return result.first->second;
  }

  return nullptr;
}

void TextureSlots::fillEmptyWithDefaults() {
  if (!this->pBaseTexture)
    this->pBaseTexture = GWhiteTexture1x1;

  if (!this->pNormalMapTexture)
    this->pNormalMapTexture = GNormalTexture1x1;

  if (!this->pMetallicRoughnessTexture)
    this->pMetallicRoughnessTexture = GWhiteTexture1x1; // GGreenTexture1x1;

  if (!this->pOcclusionTexture) {
    this->pOcclusionTexture = GWhiteTexture1x1;
  }

  if (!this->pEmissiveTexture) {
    this->pEmissiveTexture = GBlackTexture1x1;
  }
}

/*static*/
void Primitive::buildPipeline(GraphicsPipelineBuilder& builder) {
  builder.setPrimitiveType(PrimitiveType::TRIANGLES)
      .addVertexInputBinding<Vertex>()
      .addVertexAttribute(VertexAttributeType::VEC3, offsetof(Vertex, position))
      .addVertexAttribute(VertexAttributeType::VEC3, offsetof(Vertex, tangent))
      .addVertexAttribute(
          VertexAttributeType::VEC3,
          offsetof(Vertex, bitangent))
      .addVertexAttribute(VertexAttributeType::VEC3, offsetof(Vertex, normal));

  for (uint32_t i = 0; i < MAX_UV_COORDS; ++i) {
    builder.addVertexAttribute(
        VertexAttributeType::VEC2,
        offsetof(Vertex, uvs[i]));
  }

  builder.addVertexAttribute(
      VertexAttributeType::VEC4,
      offsetof(Vertex, weights));
  builder.addVertexAttribute(
      VK_FORMAT_R16G16B16A16_UINT,
      offsetof(Vertex, joints));

  builder.enableDynamicFrontFace();
}

namespace {
// merges and builds a consistent vertex buffer format from the
// gltf primitive attributes
struct VertexBufferBuilder {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  bool isSkinned = false;
  bool failed = false;

private:
  typedef std::unordered_map<std::string, int32_t>::const_iterator
      AttributeIterator;

  size_t vertexCount;
  size_t indexCount;

  CesiumGltf::AccessorView<glm::vec3> posView;
  CesiumGltf::AccessorView<glm::vec3> normView;
  CesiumGltf::AccessorView<glm::vec4> tangView;
  CesiumGltf::AccessorView<glm::vec2> uvViews[MAX_UV_COORDS];
  uint32_t uvCount;

  bool hasTangents;
  bool hasNormals;

  bool duplicateVertices;

  struct INVALID_TYPE {
    template <typename T> operator T() const {
      assert(false);
      return T();
    }
  };

public:
  VertexBufferBuilder(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      uint32_t normalMapUvIdx) {

    AttributeIterator posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end()) {
      failed = true;
      return;
    }

    posView = CesiumGltf::AccessorView<glm::vec3>(model, posIt->second);
    if (posView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      failed = true;
      return;
    }

    vertexCount = static_cast<size_t>(posView.size());

    hasTangents = false;
    hasNormals = false;

    AttributeIterator normIt = primitive.attributes.find("NORMAL");
    if (normIt != primitive.attributes.end()) {
      normView = CesiumGltf::AccessorView<glm::vec3>(model, normIt->second);
      if (normView.status() == CesiumGltf::AccessorViewStatus::Valid &&
          static_cast<size_t>(normView.size()) >= vertexCount) {
        hasNormals = true;
      }
    }

    AttributeIterator tangIt = primitive.attributes.find("TANGENT");
    if (tangIt != primitive.attributes.end()) {
      tangView = CesiumGltf::AccessorView<glm::vec4>(model, tangIt->second);
      if (tangView.status() == CesiumGltf::AccessorViewStatus::Valid &&
          static_cast<size_t>(tangView.size()) >= vertexCount) {
        hasTangents = true;
      }
    }

    uvCount = 0;
    std::array<AttributeIterator, MAX_UV_COORDS> uvIterators;
    for (size_t i = 0; i < MAX_UV_COORDS; ++i) {
      uvIterators[i] =
          primitive.attributes.find("TEXCOORD_" + std::to_string(i));
      if (uvIterators[i] == primitive.attributes.end()) {
        break;
      }

      ++uvCount;
    }

    for (uint32_t i = 0; i < uvCount; ++i) {
      uvViews[i] =
          CesiumGltf::AccessorView<glm::vec2>(model, uvIterators[i]->second);
      if (uvViews[i].status() != CesiumGltf::AccessorViewStatus::Valid ||
          static_cast<size_t>(uvViews[i].size()) < vertexCount) {
        failed = true;
        return;
      }
    }

    duplicateVertices = !hasNormals || !hasTangents;

    // Create and copy over the vertex and index buffers, duplicate vertices if
    // necessary.
    // Need to do some template nonsense to handle various types of indices,
    // joints, and weights
    int32_t indexType = 0;
    if (primitive.indices >= 0 && primitive.indices < model.accessors.size())
      indexType = model.accessors[primitive.indices].componentType;

    switch (indexType) {
    case CesiumGltf::Accessor::ComponentType::BYTE: {
      initVerticesAndIndices<int8_t>(model, primitive);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE: {
      initVerticesAndIndices<uint8_t>(model, primitive);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::SHORT: {
      initVerticesAndIndices<int16_t>(model, primitive);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT: {
      initVerticesAndIndices<uint16_t>(model, primitive);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT: {
      initVerticesAndIndices<uint32_t>(model, primitive);
      break;
    }
    default: {
      // Assume the vertex buffer is a regular triangle mesh if the
      // index buffer is invalid.
      initVerticesAndIndices<INVALID_TYPE>(model, primitive);
    }
    }

    if (!hasNormals) {
      GeometryUtilities::computeFlatNormals(vertices);
    }

    if (!hasTangents) {
      GeometryUtilities::computeTangentSpace(vertices, normalMapUvIdx);
    }
  }

private:
  template <typename TIndex>
  void initVerticesAndIndices(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive) {
    int32_t jointType = 0;
    int32_t jointsAccessorIdx = -1;

    AttributeIterator jointsIt = primitive.attributes.find("JOINTS_0");
    if (jointsIt != primitive.attributes.end() && jointsIt->second >= 0 &&
        jointsIt->second < model.accessors.size()) {
      jointsAccessorIdx = jointsIt->second;
      jointType = model.accessors[jointsAccessorIdx].componentType;
    }

    switch (jointType) {
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE: {
      initVerticesAndIndices<TIndex, glm::u8vec4>(
          model,
          primitive,
          jointsAccessorIdx);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT: {
      initVerticesAndIndices<TIndex, glm::u16vec4>(
          model,
          primitive,
          jointsAccessorIdx);
      break;
    }
    default: {
      initVerticesAndIndices<TIndex, INVALID_TYPE>(
          model,
          primitive,
          jointsAccessorIdx);
    }
    }
  }

  template <typename TIndex, typename TJoints>
  void initVerticesAndIndices(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      int32_t jointsAccessorIdx) {
    int32_t weightsType = 0;
    int32_t weightsAccessorIdx = -1;

    AttributeIterator weightsIt = primitive.attributes.find("WEIGHTS_0");
    if (weightsIt != primitive.attributes.end() && weightsIt->second >= 0 &&
        weightsIt->second < model.accessors.size()) {
      weightsAccessorIdx = weightsIt->second;
      weightsType = model.accessors[weightsAccessorIdx].componentType;
    }

    switch (weightsType) {
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE: {
      initVerticesAndIndices<TIndex, TJoints, glm::u8vec4>(
          model,
          primitive,
          jointsAccessorIdx,
          weightsAccessorIdx);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT: {
      initVerticesAndIndices<TIndex, TJoints, glm::u16vec4>(
          model,
          primitive,
          jointsAccessorIdx,
          weightsAccessorIdx);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::FLOAT: {
      initVerticesAndIndices<TIndex, TJoints, glm::vec4>(
          model,
          primitive,
          jointsAccessorIdx,
          weightsAccessorIdx);
      break;
    }
    default: {
      initVerticesAndIndices<TIndex, TJoints, INVALID_TYPE>(
          model,
          primitive,
          jointsAccessorIdx,
          weightsAccessorIdx);
    }
    }
  }

  template <typename TIndex, typename TJoints, typename TWeights>
  void initVerticesAndIndices(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      int32_t jointsAccessorIdx,
      int32_t weightsAccessorIdx) {

    CesiumGltf::AccessorView<TIndex> indexAccessor(model, primitive.indices);
    CesiumGltf::AccessorView<TJoints> jointsAccessor(model, jointsAccessorIdx);
    CesiumGltf::AccessorView<TWeights> weightsAccessor(
        model,
        weightsAccessorIdx);

    if constexpr (std::is_same<TIndex, INVALID_TYPE>::value) {
      // dummy indices
      indexCount = vertexCount;
      indices.resize(indexCount);
      for (uint32_t i = 0; i < indexCount; ++i)
        indices[i] = i;
    } else {
      if (duplicateVertices) {
        // dummy indices
        indexCount = indexAccessor.size();
        indices.resize(indexCount);
        for (uint32_t i = 0; i < indexCount; ++i)
          indices[i] = i;
      } else {
        // copy indices
        indexCount = indexAccessor.size();
        indices.resize(indexCount);
        for (uint32_t i = 0; i < indexCount; ++i)
          indices[i] = indexAccessor[i];
      }
    }

    uint32_t newVertCount = duplicateVertices ? indexCount : vertexCount;

    isSkinned = !std::is_same<TJoints, INVALID_TYPE>::value &&
                !std::is_same<TWeights, INVALID_TYPE>::value;

    vertices.resize(newVertCount);

    for (int64_t newVertIdx = 0; newVertIdx < newVertCount; ++newVertIdx) {
      Vertex& vertex = vertices[newVertIdx];
      uint32_t srcVertexIdx;
      if (std::is_same<TIndex, INVALID_TYPE>::value || !duplicateVertices) {
        srcVertexIdx = newVertIdx;
      } else {
        srcVertexIdx = indexAccessor[newVertIdx];
      }

      vertex.position = posView[srcVertexIdx];

      if (hasNormals) {
        vertex.normal = normView[srcVertexIdx];
        if (hasTangents) {
          const glm::vec4& tangent = tangView[srcVertexIdx];
          vertex.tangent = glm::vec3(tangent.x, tangent.y, tangent.z);
          vertex.bitangent =
              tangent.w * glm::cross(vertex.normal, vertex.tangent);
        }
      }

      for (uint32_t j = 0; j < uvCount; ++j) {
        vertex.uvs[j] = uvViews[j][srcVertexIdx];
      }

      if constexpr (
          !std::is_same<TJoints, INVALID_TYPE>::value &&
          !std::is_same<TWeights, INVALID_TYPE>::value) {
        vertex.joints = jointsAccessor[srcVertexIdx];

        if constexpr (std::is_same<TWeights, glm::u8vec4>::value) {
          vertex.weights = glm::vec4(weightsAccessor[srcVertexIdx]) / 255.0f;
        } else if constexpr (std::is_same<TWeights, glm::u16vec4>::value) {
          vertex.weights = glm::vec4(weightsAccessor[srcVertexIdx]) / 65535.0f;
        } else { // TWeights : vec4
          vertex.weights = weightsAccessor[srcVertexIdx];
        }
      }
    }
  }
};
} // namespace

Primitive::Primitive(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap,
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    BufferHandle jointMapHandle,
    uint32_t nodeIdx)
    : _device(app.getDevice()),
      // TODO: 
      // glm::determinant(glm::mat3(nodeTransform)) < 0.0f
      _flipFrontFace(false) {
  
  _constants.jointMapHandle = jointMapHandle.index;
  _constants.nodeIdx = nodeIdx;

  // fill some non-zero defaults
  
  _constants.baseColorFactor = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};

  _constants.normalScale = 1.0f;
  _constants.roughnessFactor = 1.0f;

  _constants.occlusionStrength = 1.0f;
  _constants.alphaCutoff = 0.5f;

  const VkPhysicalDevice& physicalDevice = app.getPhysicalDevice();

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  // If the normal map exists, we might need its UV coordinates for
  // tangent-space generation later.
  uint32_t normalMapUvIndex = 0;

  // TODO: texture map could exist on a model level, to allow for shared texture
  // resource across primitives
  std::unordered_map<const CesiumGltf::Texture*, std::shared_ptr<Texture>>
      textureMap;
  if (primitive.material >= 0 && primitive.material < model.materials.size()) {
    const CesiumGltf::Material& material = model.materials[primitive.material];
    if (material.pbrMetallicRoughness) {
      const CesiumGltf::MaterialPBRMetallicRoughness& pbr =
          *material.pbrMetallicRoughness;
      this->_textureSlots.pBaseTexture = createTexture(
          app,
          commandBuffer,
          model,
          pbr.baseColorTexture,
          textureMap,
          this->_constants.baseTextureCoordinateIndex,
          true);

      this->_constants.baseColorFactor = glm::vec4(
          static_cast<float>(pbr.baseColorFactor[0]),
          static_cast<float>(pbr.baseColorFactor[1]),
          static_cast<float>(pbr.baseColorFactor[2]),
          static_cast<float>(pbr.baseColorFactor[3]));

      this->_textureSlots.pMetallicRoughnessTexture = createTexture(
          app,
          commandBuffer,
          model,
          pbr.metallicRoughnessTexture,
          textureMap,
          this->_constants.metallicRoughnessTextureCoordinateIndex,
          false);

      this->_constants.metallicFactor = static_cast<float>(pbr.metallicFactor);
      this->_constants.roughnessFactor =
          static_cast<float>(pbr.roughnessFactor);
      // TODO: reconsider default textures!! e.g., consider no texture + factors
      // metallic factor, roughness factor
    }

    this->_textureSlots.pNormalMapTexture = createTexture(
        app,
        commandBuffer,
        model,
        material.normalTexture,
        textureMap,
        this->_constants.normalMapTextureCoordinateIndex,
        false);

    if (material.normalTexture) {
      normalMapUvIndex = this->_constants.normalMapTextureCoordinateIndex;
      this->_constants.normalScale =
          static_cast<float>(material.normalTexture->scale);
    }

    this->_textureSlots.pOcclusionTexture = createTexture(
        app,
        commandBuffer,
        model,
        material.occlusionTexture,
        textureMap,
        this->_constants.occlusionTextureCoordinateIndex,
        false);

    if (material.occlusionTexture) {
      this->_constants.occlusionStrength =
          static_cast<float>(material.occlusionTexture->strength);
    }

    this->_textureSlots.pEmissiveTexture = createTexture(
        app,
        commandBuffer,
        model,
        material.emissiveTexture,
        textureMap,
        this->_constants.emissiveTextureCoordinateIndex,
        true);

    this->_constants.emissiveFactor = glm::vec4(
        static_cast<float>(material.emissiveFactor[0]),
        static_cast<float>(material.emissiveFactor[1]),
        static_cast<float>(material.emissiveFactor[2]),
        1.0f);

    this->_constants.alphaCutoff = static_cast<float>(material.alphaCutoff);
  }

  this->_textureSlots.fillEmptyWithDefaults();

  {
    VertexBufferBuilder vbBuilder(model, primitive, normalMapUvIndex);
    if (vbBuilder.failed) {
      throw std::runtime_error(
          "Failed to create glTF vertex buffer for primitive!");
      return;
    }

    vertices = std::move(vbBuilder.vertices);
    indices = std::move(vbBuilder.indices);

    _constants.isSkinned = vbBuilder.isSkinned;
  }

  // Compute AABB
  if (vertices.size() > 0) {
    this->_aabb.min = this->_aabb.max = vertices[0].position;
    for (size_t i = 1; i < vertices.size(); ++i) {
      this->_aabb.min = glm::min(this->_aabb.min, vertices[i].position);
      this->_aabb.max = glm::max(this->_aabb.max, vertices[i].position);
    }
  } else {
    throw std::runtime_error(
        "Attempting to create a primitive with no vertices!");
  }

  this->_vertexBuffer = VertexBuffer(app, commandBuffer, std::move(vertices));
  this->_indexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));

  registerToHeap(heap);
  createConstantBuffer(app, commandBuffer, heap);
}

AABB Primitive::computeWorldAABB() const {
  const std::vector<Vertex>& vertices = this->_vertexBuffer.getVertices();

  if (vertices.empty())
    throw std::runtime_error(
        "Attempting to compute world AABB with empty vertices");

  // TODO: BROKEN
  glm::mat4 transform(1.0f);

  AABB aabb;
  aabb.min = aabb.max =
      glm::vec3(transform * glm::vec4(vertices[0].position, 1.0f));

  for (size_t i = 0; i < vertices.size(); ++i) {
    glm::vec3 worldPos(transform * glm::vec4(vertices[i].position, 1.0f));
    aabb.min = glm::min(aabb.min, worldPos);
    aabb.max = glm::max(aabb.max, worldPos);
  }

  return aabb;
}

void Primitive::registerToHeap(GlobalHeap& heap) {
  {
    TextureHandle handle = heap.registerTexture();
    heap.updateTexture(
        handle,
        this->_textureSlots.pBaseTexture->getImageView(),
        this->_textureSlots.pBaseTexture->getSampler());
    this->_constants.baseTextureHandle = handle.index;
  }

  {
    TextureHandle handle = heap.registerTexture();
    heap.updateTexture(
        handle,
        this->_textureSlots.pNormalMapTexture->getImageView(),
        this->_textureSlots.pNormalMapTexture->getSampler());
    this->_constants.normalTextureHandle = handle.index;
  }

  {
    TextureHandle handle = heap.registerTexture();
    heap.updateTexture(
        handle,
        this->_textureSlots.pMetallicRoughnessTexture->getImageView(),
        this->_textureSlots.pMetallicRoughnessTexture->getSampler());
    this->_constants.metallicRoughnessTextureHandle = handle.index;
  }

  {
    TextureHandle handle = heap.registerTexture();
    heap.updateTexture(
        handle,
        this->_textureSlots.pOcclusionTexture->getImageView(),
        this->_textureSlots.pOcclusionTexture->getSampler());
    this->_constants.occlusionTextureHandle = handle.index;
  }

  {
    TextureHandle handle = heap.registerTexture();
    heap.updateTexture(
        handle,
        this->_textureSlots.pEmissiveTexture->getImageView(),
        this->_textureSlots.pEmissiveTexture->getSampler());
    this->_constants.emissiveTextureHandle = handle.index;
  }

  {
    this->_vertexBuffer.registerToHeap(heap);
    this->_constants.vertexBufferHandle = this->_vertexBuffer.getHandle().index;

    this->_indexBuffer.registerToHeap(heap);
    this->_constants.indexBufferHandle = this->_indexBuffer.getHandle().index;
  }
}

void Primitive::createConstantBuffer(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap) {
  _constantBuffer =
      ConstantBuffer<PrimitiveConstants>(app, commandBuffer, _constants);
  _constantBuffer.registerToHeap(heap);
}
} // namespace AltheaEngine
