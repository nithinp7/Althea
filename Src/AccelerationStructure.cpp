#include "AccelerationStructure.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
// TODO: Allow for regular command buffer as well
AccelerationStructure::AccelerationStructure(
    Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const std::vector<Model>& models) {

  uint32_t primCount = 0;
  for (const Model& model : models) {
    primCount += static_cast<uint32_t>(model.getPrimitivesCount());
  }

  std::vector<AABB> aabbs;
  aabbs.reserve(primCount);
  std::vector<VkTransformMatrixKHR> transforms;
  transforms.reserve(primCount);
  for (const Model& model : models) {
    for (const Primitive& prim : model.getPrimitives()) {
      aabbs.push_back(prim.getAABB());

      const glm::mat4& primTransform = prim.getRelativeTransform();

      VkTransformMatrixKHR& transform = transforms.emplace_back();
      transform.matrix[0][0] = primTransform[0][0];
      transform.matrix[1][0] = primTransform[0][1];
      transform.matrix[2][0] = primTransform[0][2];

      transform.matrix[0][1] = primTransform[1][0];
      transform.matrix[1][1] = primTransform[1][1];
      transform.matrix[2][1] = primTransform[1][2];

      transform.matrix[0][2] = primTransform[2][0];
      transform.matrix[1][2] = primTransform[2][1];
      transform.matrix[2][2] = primTransform[2][2];

      transform.matrix[0][3] = primTransform[3][0];
      transform.matrix[1][3] = primTransform[3][1];
      transform.matrix[2][3] = primTransform[3][2];
    }
  }

  // Upload AABBs for all primitives
  VmaAllocationCreateInfo aabbBufferAllocationInfo{};
  aabbBufferAllocationInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  aabbBufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

  BufferAllocation aabbBuffer = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      sizeof(AABB) * primCount,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      aabbBufferAllocationInfo);

  {
    void* data = aabbBuffer.mapMemory();
    memcpy(data, aabbs.data(), primCount * sizeof(AABB));
    aabbBuffer.unmapMemory();
  }

  VkBufferDeviceAddressInfo aabbBufferAddrInfo{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  aabbBufferAddrInfo.buffer = aabbBuffer.getBuffer();
  VkDeviceAddress aabbBufferDevAddr =
      vkGetBufferDeviceAddress(app.getDevice(), &aabbBufferAddrInfo);

  // Upload transforms for all primitives
  VmaAllocationCreateInfo transformBufferAllocInfo{};
  transformBufferAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  transformBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  BufferAllocation transformBuffer = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      sizeof(VkTransformMatrixKHR) * primCount,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      transformBufferAllocInfo);

  {
    void* data = transformBuffer.mapMemory();
    memcpy(data, transforms.data(), primCount * sizeof(VkTransformMatrixKHR));
    transformBuffer.unmapMemory();
  }

  VkBufferDeviceAddressInfo transformBufferAddrInfo{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  transformBufferAddrInfo.buffer = transformBuffer.getBuffer();
  VkDeviceAddress transformBufferDevAddr =
      vkGetBufferDeviceAddress(app.getDevice(), &transformBufferAddrInfo);

  std::vector<VkAccelerationStructureGeometryKHR> geometries;
  geometries.reserve(primCount);

  std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRanges;
  buildRanges.reserve(primCount);

  std::vector<uint32_t> triCounts;
  triCounts.reserve(primCount);

  uint32_t primIndex = 0;
  uint32_t maxPrimCount = 0;
  for (const Model& model : models) {
    for (const Primitive& prim : model.getPrimitives()) {
      const VertexBuffer<Vertex>& vertexBuffer = prim.getVertexBuffer();
      VkBufferDeviceAddressInfo vertexBufferAddrInfo{
          VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
      vertexBufferAddrInfo.buffer = vertexBuffer.getAllocation().getBuffer();
      VkDeviceAddress vertexBufferDevAddr =
          vkGetBufferDeviceAddress(app.getDevice(), &vertexBufferAddrInfo);

      const IndexBuffer& indexBuffer = prim.getIndexBuffer();
      VkBufferDeviceAddressInfo indexBufferAddrInfo{
          VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
      indexBufferAddrInfo.buffer = indexBuffer.getAllocation().getBuffer();
      VkDeviceAddress indexBufferDevAddr =
          vkGetBufferDeviceAddress(app.getDevice(), &indexBufferAddrInfo);

      VkAccelerationStructureGeometryKHR& geometry = geometries.emplace_back();
      geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      geometry.geometry.aabbs.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
      geometry.geometry.aabbs.data.deviceAddress =
          aabbBufferDevAddr + primIndex * sizeof(AABB);
      geometry.geometry.aabbs.stride = sizeof(AABB);

      geometry.geometry.triangles.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
      geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
      geometry.geometry.triangles.vertexData.deviceAddress =
          vertexBufferDevAddr;
      geometry.geometry.triangles.vertexStride = sizeof(Vertex);
      geometry.geometry.triangles.maxVertex =
          static_cast<uint32_t>(vertexBuffer.getVertexCount() - 1);
      geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
      geometry.geometry.triangles.indexData.deviceAddress = indexBufferDevAddr;

      geometry.geometry.triangles.transformData.deviceAddress =
          transformBufferDevAddr + primIndex * sizeof(VkTransformMatrixKHR);

      VkAccelerationStructureBuildRangeInfoKHR& buildRange =
          buildRanges.emplace_back();
      buildRange.firstVertex = 0;
      buildRange.primitiveCount = indexBuffer.getIndexCount() / 3;
      buildRange.primitiveOffset = 0;
      buildRange.transformOffset = 0;

      triCounts.push_back(buildRange.primitiveCount);

      ++primIndex;
    }
  }

  VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
  buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  buildInfo.pGeometries = geometries.data();
  buildInfo.geometryCount = primCount;

  VkAccelerationStructureBuildSizesInfoKHR buildSizes{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
  Application::vkGetAccelerationStructureBuildSizesKHR(
      app.getDevice(),
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &buildInfo,
      triCounts.data(),
      &buildSizes);

  // Create backing buffer and scratch buffer for acceleration structures
  VmaAllocationCreateInfo accelStrAllocInfo{};
  accelStrAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  this->_buffer = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      buildSizes.accelerationStructureSize,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
      accelStrAllocInfo);
  BufferAllocation scratchBuffer = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      buildSizes.buildScratchSize,
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      accelStrAllocInfo);

  VkBufferDeviceAddressInfo scratchBufferAddrInfo{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  scratchBufferAddrInfo.buffer = scratchBuffer.getBuffer();
  VkDeviceAddress scratchBufferDevAddr =
      vkGetBufferDeviceAddress(app.getDevice(), &scratchBufferAddrInfo);

  buildInfo.scratchData.deviceAddress = scratchBufferDevAddr;

  VkAccelerationStructureCreateInfoKHR accelerationStructureInfo{};
  accelerationStructureInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  accelerationStructureInfo.type =
      VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  accelerationStructureInfo.buffer =
      this->_buffer.getBuffer();
  accelerationStructureInfo.offset = 0;
  accelerationStructureInfo.size = buildSizes.accelerationStructureSize;

  VkAccelerationStructureKHR accelerationStructure;
  if (Application::vkCreateAccelerationStructureKHR(
          app.getDevice(),
          &accelerationStructureInfo,
          nullptr,
          &accelerationStructure) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create acceleration structure!");
  }

  this->_accelerationStructure.set(app.getDevice(), accelerationStructure);

  buildInfo.dstAccelerationStructure = this->_accelerationStructure;

  VkAccelerationStructureBuildRangeInfoKHR* pBuildRange = buildRanges.data();
  VkAccelerationStructureBuildRangeInfoKHR** ppBuildRange = &pBuildRange;

  Application::vkCmdBuildAccelerationStructuresKHR(
      commandBuffer,
      1,
      &buildInfo,
      ppBuildRange);

  // Add task to delete all temp buffers once the commands have completed
  commandBuffer.addPostCompletionTask(
      [pAabbBuffer = new BufferAllocation(std::move(aabbBuffer)),
       pTransformBuffer = new BufferAllocation(std::move(transformBuffer)),
       pScratchBuffer = new BufferAllocation(std::move(scratchBuffer))]() {
        delete pAabbBuffer;
        delete pTransformBuffer;
        delete pScratchBuffer;
      });
}

void AccelerationStructure::AccelerationStructureDeleter::operator()(
    VkDevice device, VkAccelerationStructureKHR accelerationStructure) {
  Application::vkDestroyAccelerationStructureKHR(device, accelerationStructure, nullptr);
}
} // namespace AltheaEngine