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
    const std::vector<Material>& materialMap,
    BufferHandle jointMapHandle,
    uint32_t nodeIdx)
    : 
      // TODO:
      // glm::determinant(glm::mat3(nodeTransform)) < 0.0f
      m_flipFrontFace(false) {

  // If the normal map exists, we might need its UV coordinates for
  // tangent-space generation later.
  uint32_t normalMapUvIndex = 0; 

  PrimitiveConstants constants{};

  constants.jointMapHandle = jointMapHandle.index;
  constants.nodeIdx = nodeIdx;

   if (primitive.material >= 0 && primitive.material < model.materials.size()) {
    constants.materialHandle = materialMap[primitive.material].getHandle().index;
    normalMapUvIndex = materialMap[primitive.material].getNormalTextureCoordinateIndex();
   }
   else
   {
    // TODO - setup default material...
    constants.materialHandle = INVALID_BINDLESS_HANDLE;
   }


  VertexBufferBuilder vbBuilder(model, primitive, normalMapUvIndex);
  if (vbBuilder.failed) {
    throw std::runtime_error(
        "Failed to create glTF vertex buffer for primitive!");
    return;
  }

  std::vector<Vertex> vertices = std::move(vbBuilder.vertices);
  std::vector<uint32_t> indices = std::move(vbBuilder.indices);

  constants.isSkinned = vbBuilder.isSkinned;
  
  // Compute AABB
  if (vertices.size() > 0) {
    m_aabb.min = m_aabb.max = vertices[0].position;
    for (size_t i = 1; i < vertices.size(); ++i) {
      m_aabb.min = glm::min(m_aabb.min, vertices[i].position);
      m_aabb.max = glm::max(m_aabb.max, vertices[i].position);
    }
  } else {
    throw std::runtime_error(
        "Attempting to create a primitive with no vertices!");
  }

  m_vertexBuffer = VertexBuffer(app, commandBuffer, std::move(vertices));
  m_vertexBuffer.registerToHeap(heap);
  constants.vertexBufferHandle = m_vertexBuffer.getHandle().index;

  m_indexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));
  m_indexBuffer.registerToHeap(heap);
  constants.indexBufferHandle = m_indexBuffer.getHandle().index;

  m_constantBuffer = ConstantBuffer<PrimitiveConstants>(app, commandBuffer, constants);
  m_constantBuffer.registerToHeap(heap);
}

AABB Primitive::computeWorldAABB() const {
  const std::vector<Vertex>& vertices = m_vertexBuffer.getVertices();

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
} // namespace AltheaEngine
