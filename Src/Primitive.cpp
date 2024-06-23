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
  glm::mat4 model{};
  uint32_t constantBufferHandle{};
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
    size_t uvCount,
    bool srgb) {
  if (texture && texture->index >= 0 &&
      texture->index <= model.textures.size() &&
      static_cast<int32_t>(texture->texCoord) < uvCount) {
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
  builder
      .setPrimitiveType(PrimitiveType::TRIANGLES)
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

  builder.enableDynamicFrontFace();
}

/*static*/
void Primitive::buildMaterial(DescriptorSetLayoutBuilder& materialBuilder) {
  materialBuilder
      .addConstantsBufferBinding<PrimitiveConstants>()
      // Base color
      .addTextureBinding()
      // Normal map
      .addTextureBinding()
      // Metallic-Roughness
      .addTextureBinding()
      // Ambient Occlusion
      .addTextureBinding()
      // Emissive
      .addTextureBinding();
}

template <typename TIndex>
static void copyIndices(
    std::vector<uint32_t>& indicesOut,
    const CesiumGltf::AccessorView<TIndex>& indexAccessor) {
  assert(indexAccessor.status() == CesiumGltf::AccessorViewStatus::Valid);

  indicesOut.resize(indexAccessor.size());
  for (int64_t i = 0; i < indexAccessor.size(); ++i) {
    indicesOut[i] = static_cast<uint32_t>(indexAccessor[i]);
  }
}

template <typename TIndex>
static std::vector<Vertex> duplicateVertices(
    const CesiumGltf::AccessorView<glm::vec3>& verticesView,
    const CesiumGltf::AccessorView<TIndex>& indicesView,
    const CesiumGltf::AccessorView<glm::vec3>& normAccessor,
    const CesiumGltf::AccessorView<glm::vec4>& tangAccessor,
    const CesiumGltf::AccessorView<glm::vec2> uvAccessors[MAX_UV_COORDS],
    uint32_t uvCount) {
  std::vector<Vertex> result;
  result.resize(indicesView.size());

  bool hasNormals =
      normAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;
  bool hasTangents =
      tangAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;

  for (int64_t i = 0; i < indicesView.size(); ++i) {
    Vertex& vertex = result[i];
    size_t srcVertexIndex = static_cast<size_t>(indicesView[i]);
    vertex.position = verticesView[srcVertexIndex];

    if (hasNormals) {
      vertex.normal = normAccessor[srcVertexIndex];
      if (hasTangents) {
        const glm::vec4& tangent = tangAccessor[srcVertexIndex];
        vertex.tangent = glm::vec3(tangent.x, tangent.y, tangent.z);
        vertex.bitangent =
            tangent.w * glm::cross(vertex.normal, vertex.tangent);
      }
    }

    for (uint32_t j = 0; j < uvCount; ++j) {
      vertex.uvs[j] = uvAccessors[j][srcVertexIndex];
    }
  }

  return result;
}

static std::vector<uint32_t> createDummyIndices(uint32_t indicesCount) {
  std::vector<uint32_t> indices;
  indices.resize(indicesCount);
  for (uint32_t i = 0; i < indicesCount; ++i) {
    indices[i] = i;
  }

  return indices;
}

static std::vector<Vertex> createAndCopyVertices(
    const CesiumGltf::AccessorView<glm::vec3>& verticesAccessor,
    const CesiumGltf::AccessorView<glm::vec3>& normAccessor,
    const CesiumGltf::AccessorView<glm::vec4>& tangAccessor,
    const CesiumGltf::AccessorView<glm::vec2> uvAccessors[MAX_UV_COORDS],
    uint32_t uvCount) {
  assert(verticesAccessor.status() == CesiumGltf::AccessorViewStatus::Valid);

  std::vector<Vertex> vertices;
  vertices.resize(verticesAccessor.size());

  bool hasNormals =
      normAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;
  bool hasTangents =
      tangAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;

  for (int64_t i = 0; i < verticesAccessor.size(); ++i) {
    Vertex& vertex = vertices[i];
    vertex.position = verticesAccessor[i];

    if (hasNormals) {
      vertex.normal = normAccessor[i];
      if (hasTangents) {
        const glm::vec4& tangent = tangAccessor[i];
        vertex.tangent = glm::vec3(tangent.x, tangent.y, tangent.z);
        vertex.bitangent =
            tangent.w * glm::cross(vertex.normal, vertex.tangent);
      }
    }

    for (uint32_t j = 0; j < uvCount; ++j) {
      vertex.uvs[j] = uvAccessors[j][i];
    }
  }

  return vertices;
}

template <typename TIndex>
static void initVerticesAndIndices(
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices,
    const CesiumGltf::AccessorView<glm::vec3>& verticesAccessor,
    const CesiumGltf::AccessorView<TIndex>& indicesAccessor,
    const CesiumGltf::AccessorView<glm::vec3>& normAccessor,
    const CesiumGltf::AccessorView<glm::vec4>& tangAccessor,
    const CesiumGltf::AccessorView<glm::vec2> uvAccessors[MAX_UV_COORDS],
    uint32_t uvCount,
    bool shouldDuplicateVertices) {
  if (shouldDuplicateVertices) {
    vertices = duplicateVertices(
        verticesAccessor,
        indicesAccessor,
        normAccessor,
        tangAccessor,
        uvAccessors,
        uvCount);
    indices = createDummyIndices(static_cast<uint32_t>(indicesAccessor.size()));
  } else {
    vertices = createAndCopyVertices(
        verticesAccessor,
        normAccessor,
        tangAccessor,
        uvAccessors,
        uvCount);
    copyIndices(indices, indicesAccessor);
  }
}

typedef std::unordered_map<std::string, int32_t>::const_iterator
    AttributeIterator;

Primitive::Primitive(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const glm::mat4& nodeTransform,
    DescriptorSetAllocator* pMaterialAllocator)
    : _device(app.getDevice()),
      _transform(nodeTransform),
      _flipFrontFace(glm::determinant(glm::mat3(nodeTransform)) < 0.0f),
      _material() {
  const VkPhysicalDevice& physicalDevice = app.getPhysicalDevice();

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  AttributeIterator posIt = primitive.attributes.find("POSITION");
  if (posIt == primitive.attributes.end()) {
    return;
  }

  CesiumGltf::AccessorView<glm::vec3> posView(model, posIt->second);
  if (posView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    return;
  }

  size_t vertexCount = static_cast<size_t>(posView.size());

  bool hasTangents = false;
  bool hasNormals = false;
  bool hasNormalMap = false;

  AttributeIterator normIt = primitive.attributes.find("NORMAL");
  CesiumGltf::AccessorView<glm::vec3> normView;
  if (normIt != primitive.attributes.end()) {
    normView = CesiumGltf::AccessorView<glm::vec3>(model, normIt->second);
    if (normView.status() == CesiumGltf::AccessorViewStatus::Valid &&
        static_cast<size_t>(normView.size()) >= vertexCount) {
      hasNormals = true;
    }
  }

  AttributeIterator tangIt = primitive.attributes.find("TANGENT");
  CesiumGltf::AccessorView<glm::vec4> tangView;
  if (tangIt != primitive.attributes.end()) {
    tangView = CesiumGltf::AccessorView<glm::vec4>(model, tangIt->second);
    if (tangView.status() == CesiumGltf::AccessorViewStatus::Valid &&
        static_cast<size_t>(tangView.size()) >= vertexCount) {
      hasTangents = true;
    }
  }

  uint32_t uvCount = 0;
  std::array<AttributeIterator, MAX_UV_COORDS> uvIterators;
  for (size_t i = 0; i < MAX_UV_COORDS; ++i) {
    uvIterators[i] = primitive.attributes.find("TEXCOORD_" + std::to_string(i));
    if (uvIterators[i] == primitive.attributes.end()) {
      break;
    }

    ++uvCount;
  }

  CesiumGltf::AccessorView<glm::vec2> uvViews[MAX_UV_COORDS];
  for (uint32_t i = 0; i < uvCount; ++i) {
    uvViews[i] =
        CesiumGltf::AccessorView<glm::vec2>(model, uvIterators[i]->second);
    if (uvViews[i].status() != CesiumGltf::AccessorViewStatus::Valid ||
        static_cast<size_t>(uvViews[i].size()) < vertexCount) {
      return;
    }
  }

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
          uvCount,
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
          uvCount,
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
        uvCount,
        false);

    if (material.normalTexture) {
      hasNormalMap = true;
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
        uvCount,
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
        uvCount,
        true);

    this->_constants.emissiveFactor = glm::vec4(
        static_cast<float>(material.emissiveFactor[0]),
        static_cast<float>(material.emissiveFactor[1]),
        static_cast<float>(material.emissiveFactor[2]),
        1.0f);

    this->_constants.alphaCutoff = static_cast<float>(material.alphaCutoff);
  }

  this->_textureSlots.fillEmptyWithDefaults();

  bool needsTangents = hasNormalMap || hasTangents; // || alwaysWantTangents;
  bool duplicateVertices = !hasNormals || (needsTangents && !hasTangents);

  // Create and copy over the vertex and index buffers, duplicate vertices if
  // necessary.
  bool validIndices =
      primitive.indices >= 0 && primitive.indices < model.accessors.size();
  if (validIndices) {
    const CesiumGltf::Accessor& indexAccessorGltf =
        model.accessors[primitive.indices];
    if (indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::BYTE) {
      CesiumGltf::AccessorView<int8_t> indexAccessor(model, primitive.indices);
      initVerticesAndIndices(
          vertices,
          indices,
          posView,
          indexAccessor,
          normView,
          tangView,
          uvViews,
          uvCount,
          duplicateVertices);
    } else if (
        indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE) {
      CesiumGltf::AccessorView<uint8_t> indexAccessor(model, primitive.indices);
      initVerticesAndIndices(
          vertices,
          indices,
          posView,
          indexAccessor,
          normView,
          tangView,
          uvViews,
          uvCount,
          duplicateVertices);
    } else if (
        indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::SHORT) {
      CesiumGltf::AccessorView<int16_t> indexAccessor(model, primitive.indices);
      initVerticesAndIndices(
          vertices,
          indices,
          posView,
          indexAccessor,
          normView,
          tangView,
          uvViews,
          uvCount,
          duplicateVertices);
    } else if (
        indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT) {
      CesiumGltf::AccessorView<uint16_t> indexAccessor(
          model,
          primitive.indices);
      initVerticesAndIndices(
          vertices,
          indices,
          posView,
          indexAccessor,
          normView,
          tangView,
          uvViews,
          uvCount,
          duplicateVertices);
    } else if (
        indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::UNSIGNED_INT) {
      CesiumGltf::AccessorView<uint32_t> indexAccessor(
          model,
          primitive.indices);
      initVerticesAndIndices(
          vertices,
          indices,
          posView,
          indexAccessor,
          normView,
          tangView,
          uvViews,
          uvCount,
          duplicateVertices);
    } else {
      validIndices = false;
    }
  }

  // Assume the vertex buffer is a regular triangle mesh if the
  // index buffer is invalid.
  if (!validIndices) {
    vertices =
        createAndCopyVertices(posView, normView, tangView, uvViews, uvCount);
    indices = createDummyIndices(static_cast<uint32_t>(vertexCount));
  }

  if (!hasNormals) {
    GeometryUtilities::computeFlatNormals(vertices);
  }

  if (!hasTangents && needsTangents) {
    GeometryUtilities::computeTangentSpace(vertices, normalMapUvIndex);
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

  if (pMaterialAllocator) {
    this->_material = Material(app, *pMaterialAllocator);
    this->_material.assign()
        .bindInlineConstants(this->_constants)
        .bindTexture(*this->_textureSlots.pBaseTexture)
        .bindTexture(*this->_textureSlots.pNormalMapTexture)
        .bindTexture(*this->_textureSlots.pMetallicRoughnessTexture)
        .bindTexture(*this->_textureSlots.pOcclusionTexture)
        .bindTexture(*this->_textureSlots.pEmissiveTexture);
  }

  // If the material allocator is null, assume we are using bindless textures
}

AABB Primitive::computeWorldAABB() const {
  const std::vector<Vertex>& vertices = this->_vertexBuffer.getVertices();

  if (vertices.empty())
    throw std::runtime_error(
        "Attempting to compute world AABB with empty vertices");

  AABB aabb;
  aabb.min = aabb.max =
      glm::vec3(_transform * glm::vec4(vertices[0].position, 1.0f));

  for (size_t i = 0; i < vertices.size(); ++i) {
    glm::vec3 worldPos(_transform * glm::vec4(vertices[i].position, 1.0f));
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

void Primitive::createConstantBuffer(const Application& app, SingleTimeCommandBuffer& commandBuffer, GlobalHeap& heap) {
  _constantBuffer = ConstantBuffer<PrimitiveConstants>(app, commandBuffer, _constants);
  _constantBuffer.registerToHeap(heap);
}

void Primitive::draw(const DrawContext& context) const {
  context.setFrontFaceDynamic(
      this->_flipFrontFace ? VK_FRONT_FACE_CLOCKWISE
                           : VK_FRONT_FACE_COUNTER_CLOCKWISE);
  // TODO: This no longer works for non-bindless
  // if (this->_material)
  //   context.bindDescriptorSets(this->_material);
  // else
  context.bindDescriptorSets();
  context.updatePushConstants(
      PrimitivePushConstants{
          _transform,
          _constantBuffer.getHandle().index},
      0);
  context.drawIndexed(this->_vertexBuffer, this->_indexBuffer);
}
} // namespace AltheaEngine
