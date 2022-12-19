#include "Primitive.h"
#include "Utilities.h"
#include "Application.h"
#include "ModelViewProjection.h"
#include "DefaultTextures.h"
#include "GeometryUtilities.h"

#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/TextureInfo.h>

#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace AltheaEngine {
static std::shared_ptr<Texture> createTexture(
    const Application& app,
    const CesiumGltf::Model& model,
    const std::optional<CesiumGltf::TextureInfo>& texture,
    std::unordered_map<const CesiumGltf::Texture*, std::shared_ptr<Texture>>& textureMap,
    int32_t& textureCoordinateIndexConstant,
    size_t uvCount) {
  if (texture && texture->index >= 0 && texture->index <= model.textures.size() && 
      static_cast<int32_t>(texture->texCoord) < uvCount) {
    textureCoordinateIndexConstant = static_cast<int32_t>(texture->texCoord);

    const CesiumGltf::Texture& gltfTexture = model.textures[texture->index];
    auto textureIt = textureMap.find(&gltfTexture);
    if (textureIt != textureMap.end()) {
      return textureIt->second;
    }

    auto result = textureMap.emplace(&gltfTexture, std::make_shared<Texture>(app, model, model.textures[texture->index]));
    return result.first->second;
  }

  return nullptr;
}

void TextureSlots::fillEmptyWithDefaults() {
  if (!this->pBaseTexture)
    this->pBaseTexture = GWhiteTexture1x1;
  
  if (!this->pNormalMapTexture)
    this->pNormalMapTexture = GNormalTexture1x1;
}

/*static*/
VkVertexInputBindingDescription Vertex::getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

/*static*/
std::array<VkVertexInputAttributeDescription, 4 + MAX_UV_COORDS> 
    Vertex::getAttributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 4 + MAX_UV_COORDS> 
      attributeDescriptions;
  
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, position);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, tangent);
  
  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(Vertex, bitangent);
  
  attributeDescriptions[3].binding = 0;
  attributeDescriptions[3].location = 3;
  attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[3].offset = offsetof(Vertex, normal);
  
  for (uint32_t i = 0; i < MAX_UV_COORDS; ++i) {
    attributeDescriptions[4 + i].binding = 0;
    attributeDescriptions[4 + i].location = 4 + i;
    attributeDescriptions[4 + i].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[4 + i].offset = offsetof(Vertex, uvs[i]);
  }

  return attributeDescriptions;
}

/*static*/
std::array<VkDescriptorSetLayoutBinding, 4> Primitive::getBindings() {
  std::array<VkDescriptorSetLayoutBinding, 4> bindings;

  VkDescriptorSetLayoutBinding& uboLayoutBinding = bindings[0];
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.pImmutableSamplers = nullptr;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding& constantsLayoutBinding = bindings[1];
  constantsLayoutBinding.binding = 1;
  constantsLayoutBinding.descriptorCount = sizeof(PrimitiveConstants);
  constantsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
  constantsLayoutBinding.pImmutableSamplers = nullptr;
  constantsLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding& baseTextureLayoutBinding = bindings[2];
  baseTextureLayoutBinding.binding = 2;
  baseTextureLayoutBinding.descriptorCount = 1;
  baseTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  baseTextureLayoutBinding.pImmutableSamplers = nullptr;
  baseTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding& normalTextureLayoutBinding = bindings[3];
  normalTextureLayoutBinding.binding = 3;
  normalTextureLayoutBinding.descriptorCount = 1;
  normalTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  normalTextureLayoutBinding.pImmutableSamplers = nullptr;
  normalTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  return bindings;
}

/*static*/ 
std::array<VkDescriptorPoolSize, 4> Primitive::getPoolSizes(uint32_t descriptorCount) {
  std::array<VkDescriptorPoolSize, 4> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = descriptorCount;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
  poolSizes[1].descriptorCount = sizeof(PrimitiveConstants) * descriptorCount;
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[2].descriptorCount = descriptorCount;
  poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[3].descriptorCount = descriptorCount;

  return poolSizes;
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

  bool hasNormals = normAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;
  bool hasTangents = tangAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;

  for (int64_t i = 0; i < indicesView.size(); ++i) {
    Vertex& vertex = result[i];
    size_t srcVertexIndex = static_cast<size_t>(indicesView[i]);
    vertex.position = verticesView[srcVertexIndex];

    if (hasNormals) {
      vertex.normal = normAccessor[srcVertexIndex];
      if (hasTangents) {
        const glm::vec4& tangent = tangAccessor[srcVertexIndex];
        vertex.tangent = glm::vec3(tangent.x, tangent.y, tangent.z);
        vertex.bitangent = tangent.w * glm::cross(vertex.normal, vertex.tangent);
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

  bool hasNormals = normAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;
  bool hasTangents = tangAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;

  for (int64_t i = 0; i < verticesAccessor.size(); ++i) {
    Vertex& vertex = vertices[i];
    vertex.position = verticesAccessor[i];
    
    if (hasNormals) {
      vertex.normal = normAccessor[i];
      if (hasTangents) {
        const glm::vec4& tangent = tangAccessor[i];
        vertex.tangent = glm::vec3(tangent.x, tangent.y, tangent.z);
        vertex.bitangent = tangent.w * glm::cross(vertex.normal, vertex.tangent);
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
    vertices = 
        duplicateVertices(
          verticesAccessor, 
          indicesAccessor,
          normAccessor,
          tangAccessor,
          uvAccessors,
          uvCount);
    indices = createDummyIndices(static_cast<uint32_t>(indicesAccessor.size()));
  } else {
    vertices = 
        createAndCopyVertices(
          verticesAccessor,
          normAccessor,
          tangAccessor,
          uvAccessors,
          uvCount);
    copyIndices(indices, indicesAccessor);
  }
}

typedef std::unordered_map<std::string, int32_t>::const_iterator AttributeIterator;

Primitive::Primitive(
    const Application& app,
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const glm::mat4& nodeTransform) 
  : _device(app.getDevice()),
    _relativeTransform(nodeTransform),
    _flipFrontFace(glm::determinant(nodeTransform) < 0.0f) {

  const VkPhysicalDevice& physicalDevice = app.getPhysicalDevice();

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
    uvIterators[i] = 
        primitive.attributes.find("TEXCOORD_" + std::to_string(i));
    if (uvIterators[i] == primitive.attributes.end()) {
      break;
    }

    ++uvCount;
  }

  CesiumGltf::AccessorView<glm::vec2> uvViews[MAX_UV_COORDS];
  for (uint32_t i = 0; i < uvCount; ++i) {
    uvViews[i] = CesiumGltf::AccessorView<glm::vec2>(model, uvIterators[i]->second);
    if (uvViews[i].status() != CesiumGltf::AccessorViewStatus::Valid ||
        static_cast<size_t>(uvViews[i].size()) < vertexCount) {
      return;
    }
  }

  // If the normal map exists, we might need its UV coordinates for tangent-space
  // generation later.
  uint32_t normalMapUvIndex = 0;

  std::unordered_map<const CesiumGltf::Texture*, std::shared_ptr<Texture>> textureMap;
  if (primitive.material >= 0 && primitive.material < model.materials.size()) {
    const CesiumGltf::Material& material = model.materials[primitive.material];
    if (material.pbrMetallicRoughness) {
      const CesiumGltf::MaterialPBRMetallicRoughness& pbr = 
          *material.pbrMetallicRoughness;
      this->_textureSlots.pBaseTexture = createTexture(
          app, 
          model, 
          pbr.baseColorTexture, 
          textureMap, 
          this->_constants.baseTextureCoordinateIndex, 
          uvCount);
      // TODO: pbr.baseColorFactor 
    }

    this->_textureSlots.pNormalMapTexture = createTexture(
        app, 
        model, 
        material.normalTexture, 
        textureMap, 
        this->_constants.normalMapTextureCoordinateIndex, 
        uvCount);

    if (material.normalTexture) {
      hasNormalMap = true;
      normalMapUvIndex = this->_constants.normalMapTextureCoordinateIndex;
    }
  }

  this->_textureSlots.fillEmptyWithDefaults();

  bool needsTangents = hasNormalMap || hasTangents; // || alwaysWantTangents;
  bool duplicateVertices = !hasNormals || (needsTangents && !hasTangents);

  // Create and copy over the vertex and index buffers, duplicate vertices if necessary.
  bool validIndices = primitive.indices >= 0 && primitive.indices < model.accessors.size();
  if (validIndices) {
    const CesiumGltf::Accessor& indexAccessorGltf = model.accessors[primitive.indices];
    if (indexAccessorGltf.componentType == CesiumGltf::Accessor::ComponentType::BYTE) {
      CesiumGltf::AccessorView<int8_t> indexAccessor(model, primitive.indices);
      initVerticesAndIndices(
          this->_vertices, 
          this->_indices, 
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
          this->_vertices, 
          this->_indices, 
          posView, 
          indexAccessor, 
          normView,
          tangView,
          uvViews,
          uvCount,
          duplicateVertices);
    } else if (
        indexAccessorGltf.componentType == CesiumGltf::Accessor::ComponentType::SHORT) {
      CesiumGltf::AccessorView<int16_t> indexAccessor(model, primitive.indices);
      initVerticesAndIndices(
          this->_vertices, 
          this->_indices, 
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
      CesiumGltf::AccessorView<uint16_t> indexAccessor(model, primitive.indices);
      initVerticesAndIndices(
          this->_vertices, 
          this->_indices, 
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
      CesiumGltf::AccessorView<uint32_t> indexAccessor(model, primitive.indices);
      initVerticesAndIndices(
          this->_vertices, 
          this->_indices, 
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
    this->_vertices = createAndCopyVertices(posView, normView, tangView, uvViews, uvCount);
    this->_indices = createDummyIndices(static_cast<uint32_t>(vertexCount));
  }

  if (!hasNormals) {
    GeometryUtilities::computeFlatNormals(this->_vertices);
  }

  if (!hasTangents && needsTangents) {
    GeometryUtilities::computeTangentSpace(this->_vertices, normalMapUvIndex);
  }

  const VkExtent2D& extent = app.getSwapChainExtent();

  app.createVertexBuffer(
      (const void*)this->_vertices.data(), 
      sizeof(Vertex) * this->_vertices.size(),
      this->_vertexBuffer,
      this->_vertexBufferMemory);
  app.createIndexBuffer(
      (const void*)this->_indices.data(),
      sizeof(uint32_t) * this->_indices.size(),
      this->_indexBuffer,
      this->_indexBufferMemory);
  app.createUniformBuffers(
      sizeof(ModelViewProjection),
      this->_uniformBuffers,
      this->_uniformBuffersMemory);

  this->_descriptorSets.resize(app.getMaxFramesInFlight());
}

Primitive::Primitive(Primitive&& rhs) noexcept
  : _device(rhs._device),
    _relativeTransform(rhs._relativeTransform),
    _vertices(std::move(rhs._vertices)),
    _indices(std::move(rhs._indices)),
    _constants(rhs._constants),
    _textureSlots(std::move(rhs._textureSlots)),
    _vertexBuffer(rhs._vertexBuffer),
    _indexBuffer(rhs._indexBuffer),
    _uniformBuffers(std::move(rhs._uniformBuffers)),
    _vertexBufferMemory(rhs._vertexBufferMemory),
    _indexBufferMemory(rhs._indexBufferMemory),
    _uniformBuffersMemory(std::move(rhs._uniformBuffersMemory)),
    _descriptorSets(std::move(rhs._descriptorSets)) {
  rhs._needsDestruction = false;
}

void Primitive::updateUniforms(
    const glm::mat4& parentTransform,
    const glm::mat4& view, 
    const glm::mat4& projection, 
    uint32_t currentFrame) const {
  ModelViewProjection mvp{};
  mvp.model = parentTransform * this->_relativeTransform;
  mvp.view = view;
  mvp.projection = projection;

  void* data;
  vkMapMemory(this->_device, this->_uniformBuffersMemory[currentFrame], 0, sizeof(ModelViewProjection), 0, &data);
  memcpy(data, &mvp, sizeof(ModelViewProjection));
  vkUnmapMemory(this->_device, this->_uniformBuffersMemory[currentFrame]);
}

void Primitive::assignDescriptorSets(std::vector<VkDescriptorSet>& availableDescriptorSets) {
  for (size_t i = 0; i < this->_descriptorSets.size(); ++i) {
    VkDescriptorSet& descriptorSet = this->_descriptorSets[i];
    const VkBuffer& uniformBuffer = this->_uniformBuffers[i];

    if (availableDescriptorSets.size() == 0) {
      throw std::runtime_error("Ran out of descriptor sets when assigning to primitive!");
    }

    descriptorSet = availableDescriptorSets.back();
    availableDescriptorSets.pop_back();

    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = uniformBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(ModelViewProjection);

    std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &uniformBufferInfo;
    descriptorWrites[0].pImageInfo = nullptr;
    descriptorWrites[0].pTexelBufferView = nullptr;
    
    VkWriteDescriptorSetInlineUniformBlock inlineConstantsWrite{};
    inlineConstantsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK;
    inlineConstantsWrite.dataSize = sizeof(PrimitiveConstants);
    inlineConstantsWrite.pData = &_constants;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
    descriptorWrites[1].descriptorCount = sizeof(PrimitiveConstants);
    descriptorWrites[1].pBufferInfo = nullptr;
    descriptorWrites[1].pImageInfo = nullptr;
    descriptorWrites[1].pTexelBufferView = nullptr;
    descriptorWrites[1].pNext = &inlineConstantsWrite;

    VkDescriptorImageInfo baseTextureImageInfo{};
    baseTextureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    baseTextureImageInfo.imageView = _textureSlots.pBaseTexture->getImageView();
    baseTextureImageInfo.sampler = _textureSlots.pBaseTexture->getSampler();
    
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = nullptr;
    descriptorWrites[2].pImageInfo = &baseTextureImageInfo;
    descriptorWrites[2].pTexelBufferView = nullptr;

    VkDescriptorImageInfo normalTextureImageInfo{};
    normalTextureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalTextureImageInfo.imageView = _textureSlots.pNormalMapTexture->getImageView();
    normalTextureImageInfo.sampler = _textureSlots.pNormalMapTexture->getSampler();
    
    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = descriptorSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = nullptr;
    descriptorWrites[3].pImageInfo = &normalTextureImageInfo;
    descriptorWrites[3].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(this->_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }
}

void Primitive::render(
    const VkCommandBuffer& commandBuffer, 
    const VkPipelineLayout& pipelineLayout, 
    uint32_t currentFrame) const {
  VkDeviceSize offset = 0;
  vkCmdSetFrontFace(commandBuffer, this->_flipFrontFace ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE);
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &this->_vertexBuffer, &offset);
  vkCmdBindIndexBuffer(commandBuffer, this->_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(
      commandBuffer, 
      VK_PIPELINE_BIND_POINT_GRAPHICS, 
      pipelineLayout, 
      0, 
      1, 
      &this->_descriptorSets[currentFrame], 
      0, 
      nullptr);
  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->_indices.size()), 1, 0, 0, 0);
}

Primitive::~Primitive() noexcept {
  if (!this->_needsDestruction) {
    return;
  }

  vkDestroyBuffer(this->_device, this->_vertexBuffer, nullptr);
  vkDestroyBuffer(this->_device, this->_indexBuffer, nullptr);

  for (VkBuffer& uniformBuffer : this->_uniformBuffers) {
    vkDestroyBuffer(this->_device, uniformBuffer, nullptr);
  }

  vkFreeMemory(this->_device, this->_vertexBufferMemory, nullptr);
  vkFreeMemory(this->_device, this->_indexBufferMemory, nullptr);

  for (VkDeviceMemory& uniformBufferMemory : this->_uniformBuffersMemory) {
    vkFreeMemory(this->_device, uniformBufferMemory, nullptr);
  }
}
} // namespace AltheaEngine
