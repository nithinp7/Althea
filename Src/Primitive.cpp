#include "Primitive.h"
#include "Utilities.h"
#include "Application.h"
#include "ModelViewProjection.h"

#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>

#include <glm/gtc/matrix_transform.hpp>
// TODO: do this in cmake
#define GLM_FORCE_RADIANS

/*static*/
VkVertexInputBindingDescription Vertex::getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

/*static*/
std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
  
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, position);
  
  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, normal);
  
  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(Vertex, uv);

  return attributeDescriptions;
}

template <typename TIndexType>
static void copyIndices(
    std::vector<uint32_t>& indicesOut, 
    const CesiumGltf::AccessorView<TIndexType>& indexAccessor) {
  if (indexAccessor.status() != CesiumGltf::AccessorViewStatus::Valid) {
    return;
  }

  indicesOut.resize(indexAccessor.size());
  for (size_t i = 0; i < indexAccessor.size(); ++i) {
    indicesOut[i] = static_cast<uint32_t>(indexAccessor[i]);
  }
}

Primitive::Primitive(
    const Application& app,
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const glm::mat4& nodeTransform) 
  : _device(app.getDevice()),
    _relativeTransform(nodeTransform) {

  const VkPhysicalDevice& physicalDevice = app.getPhysicalDevice();

  auto posIt = primitive.attributes.find("POSITION");
  auto normIt = primitive.attributes.find("NORMAL");
  // TODO:
  //auto texCoordIt = primitive.attributes.find("TEXCOORD_0");

  if (posIt == primitive.attributes.end() ||
      normIt == primitive.attributes.end()) {
    return;
  }

  CesiumGltf::AccessorView<glm::vec3> posView(model, posIt->second);
  CesiumGltf::AccessorView<glm::vec3> normView(model, normIt->second);
  if (posView.status() != CesiumGltf::AccessorViewStatus::Valid ||
      normView.status() != CesiumGltf::AccessorViewStatus::Valid ||
      posView.size() != normView.size()) {
    return;
  }

  this->_vertices.resize(static_cast<size_t>(posView.size()));
  for (size_t i = 0; i < this->_vertices.size(); ++i) {
    Vertex& vertex = this->_vertices[i];
    vertex.position = posView[i];
    vertex.normal = normView[i]; 
  }

  if (primitive.indices < 0 ||
      primitive.indices >= model.accessors.size()) {
    this->_indices.resize(this->_vertices.size());
    for (uint32_t i = 0; i < this->_indices.size(); ++i) {
      this->_indices[i] = i;
    }
  } else {
    const CesiumGltf::Accessor& indexAccessorGltf = model.accessors[primitive.indices];
    if (indexAccessorGltf.componentType == CesiumGltf::Accessor::ComponentType::BYTE) {
      CesiumGltf::AccessorView<int8_t> indexAccessor(model, primitive.indices);
      copyIndices(this->_indices, indexAccessor);
    } else if (
        indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE) {
      CesiumGltf::AccessorView<uint8_t> indexAccessor(model, primitive.indices);
      copyIndices(this->_indices, indexAccessor);
    } else if (
        indexAccessorGltf.componentType == CesiumGltf::Accessor::ComponentType::SHORT) {
      CesiumGltf::AccessorView<int16_t> indexAccessor(model, primitive.indices);
      copyIndices(this->_indices, indexAccessor);
    } else if (
        indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT) {
      CesiumGltf::AccessorView<uint16_t> indexAccessor(model, primitive.indices);
      copyIndices(this->_indices, indexAccessor);
    } else if (
        indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::UNSIGNED_INT) {
      CesiumGltf::AccessorView<uint32_t> indexAccessor(model, primitive.indices);
      copyIndices(this->_indices, indexAccessor);
    }
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

#include <iostream>
Primitive::Primitive(Primitive&& rhs) noexcept
  : _device(rhs._device),
    _relativeTransform(rhs._relativeTransform),
    _vertices(std::move(rhs._vertices)),
    _indices(std::move(rhs._indices)),
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

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(ModelViewProjection);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(this->_device, 1, &descriptorWrite, 0, nullptr);
  }
}

void Primitive::render(
    const VkCommandBuffer& commandBuffer, 
    const VkPipelineLayout& pipelineLayout, 
    uint32_t currentFrame) const {
  VkDeviceSize offset = 0;
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

  vkFreeMemory(this->_device, this->_vertexBufferMemory, nullptr);
  vkFreeMemory(this->_device, this->_indexBufferMemory, nullptr);  

  for (VkBuffer& uniformBuffer : this->_uniformBuffers) {
    vkDestroyBuffer(this->_device, uniformBuffer, nullptr);
  }

  for (VkDeviceMemory& uniformBufferMemory : this->_uniformBuffersMemory) {
    vkFreeMemory(this->_device, uniformBufferMemory, nullptr);
  }
}