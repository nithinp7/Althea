#include "Primitive.h"
#include "Utilities.h"

#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>

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
    const VkDevice& device,
    const VkPhysicalDevice& physicalDevice,
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive) {
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

  VkBufferCreateInfo vertexBufferInfo{};
  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.size = sizeof(Vertex) * this->_vertices.size(); 
  vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &vertexBufferInfo, nullptr, &this->_vertexBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create vertex buffer!");
  }

  VkBufferCreateInfo indexBufferInfo{};
  indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  indexBufferInfo.size = sizeof(uint32_t) * this->_indices.size();
  indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &indexBufferInfo, nullptr, &this->_indexBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create index buffer!");
  }

  VkMemoryRequirements vertexBufferMemRequirements;
  vkGetBufferMemoryRequirements(device, this->_vertexBuffer, &vertexBufferMemRequirements);

  VkMemoryAllocateInfo vertexBufferAllocInfo{};
  vertexBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  vertexBufferAllocInfo.allocationSize = vertexBufferMemRequirements.size;
  vertexBufferAllocInfo.memoryTypeIndex = 
      Utilities::findMemoryType(
        physicalDevice, 
        vertexBufferMemRequirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); 

  if (vkAllocateMemory(device, &vertexBufferAllocInfo, nullptr, &this->_vertexBufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate vertex buffer memory!");
  }

  VkMemoryRequirements indexBufferMemRequirements;
  vkGetBufferMemoryRequirements(device, this->_indexBuffer, &indexBufferMemRequirements);
  
  VkMemoryAllocateInfo indexBufferAllocInfo{};
  indexBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  indexBufferAllocInfo.allocationSize = indexBufferMemRequirements.size;
  indexBufferAllocInfo.memoryTypeIndex = 
      Utilities::findMemoryType(
        physicalDevice,
        indexBufferMemRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); 

  if (vkAllocateMemory(device, &indexBufferAllocInfo, nullptr, &this->_indexBufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate index buffer memory!");
  }

  vkBindBufferMemory(device, this->_vertexBuffer, this->_vertexBufferMemory, 0);
  vkBindBufferMemory(device, this->_indexBuffer, this->_indexBufferMemory, 0);

  void* vertexData;
  vkMapMemory(device, this->_vertexBufferMemory, 0, vertexBufferInfo.size, 0, &vertexData);
  std::memcpy(vertexData, this->_vertices.data(), (size_t) vertexBufferInfo.size);
  vkUnmapMemory(device, this->_vertexBufferMemory);

  void* indexData;
  vkMapMemory(device, this->_indexBufferMemory, 0, indexBufferInfo.size, 0, &indexData);
  std::memcpy(indexData, this->_indices.data(), (size_t) indexBufferInfo.size);
  vkUnmapMemory(device, this->_indexBufferMemory);
}

void Primitive::render(const VkCommandBuffer& commandBuffer) const {
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &this->_vertexBuffer, &offset);
  vkCmdBindIndexBuffer(commandBuffer, this->_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->_indices.size()), 1, 0, 0, 0);
}

void Primitive::destroy(const VkDevice& device) {
  vkDestroyBuffer(device, this->_vertexBuffer, nullptr);
  vkDestroyBuffer(device, this->_indexBuffer, nullptr);

  vkFreeMemory(device, this->_vertexBufferMemory, nullptr);
  vkFreeMemory(device, this->_indexBufferMemory, nullptr);  
}