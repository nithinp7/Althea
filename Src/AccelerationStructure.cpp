#include "AccelerationStructure.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
// TODO: Allow for regular command buffer as well
// TODO: Separate out BLAS and TLAS building to make this more readable
AccelerationStructure::AccelerationStructure(
    Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const std::vector<Model>& models) {
  uint32_t primCount = 0;
  for (const Model& model : models) {
    primCount += static_cast<uint32_t>(model.getPrimitivesCount());
  }

  // Need to make sure vertex and index buffers are finished transferring
  // before reading from them to build the AS.
  {
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0,
        1,
        &barrier,
        0,
        nullptr,
        0,
        nullptr);
  }

  // std::vector<AABB> aabbs;
  // aabbs.reserve(primCount);
  std::vector<VkTransformMatrixKHR> transforms;
  transforms.reserve(primCount);
  for (const Model& model : models) {
    for (const Primitive& prim : model.getPrimitives()) {
      glm::mat4 primTransform = prim.computeWorldTransform();

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

  // Upload transforms for all primitives
  VmaAllocationCreateInfo transformBufferAllocInfo{};
  transformBufferAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  transformBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  this->_transformBuffer = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      sizeof(VkTransformMatrixKHR) * primCount,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      transformBufferAllocInfo);

  {
    void* data = this->_transformBuffer.mapMemory();
    memcpy(data, transforms.data(), primCount * sizeof(VkTransformMatrixKHR));
    this->_transformBuffer.unmapMemory();
  }

  VkBufferDeviceAddressInfo transformBufferAddrInfo{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  transformBufferAddrInfo.buffer = this->_transformBuffer.getBuffer();
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

      geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;

      VkAccelerationStructureBuildRangeInfoKHR& buildRange =
          this->_blasBuildRanges.emplace_back();
      buildRange.firstVertex = 0;
      buildRange.primitiveCount = indexBuffer.getIndexCount() / 3;
      buildRange.primitiveOffset = 0;
      buildRange.transformOffset = 0;

      triCounts.push_back(buildRange.primitiveCount);

      ++primIndex;
    }
  }

  VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfo{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
  blasBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  blasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  blasBuildInfo.pGeometries = geometries.data();
  blasBuildInfo.geometryCount = primCount;

  VkAccelerationStructureBuildSizesInfoKHR blasBuildSizes{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
  Application::vkGetAccelerationStructureBuildSizesKHR(
      app.getDevice(),
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &blasBuildInfo,
      triCounts.data(),
      &blasBuildSizes);

  // Create backing buffer and scratch buffer for BLAS
  VmaAllocationCreateInfo accelStrAllocInfo{};
  accelStrAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  this->_blasBuffer = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      blasBuildSizes.accelerationStructureSize,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      accelStrAllocInfo);

  BufferAllocation blasScratchBuffer = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      blasBuildSizes.buildScratchSize,
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      accelStrAllocInfo);

  VkBufferDeviceAddressInfo blasScratchBufferAddrInfo{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  blasScratchBufferAddrInfo.buffer = blasScratchBuffer.getBuffer();
  VkDeviceAddress blasScratchBufferDevAddr =
      vkGetBufferDeviceAddress(app.getDevice(), &blasScratchBufferAddrInfo);

  blasBuildInfo.scratchData.deviceAddress = blasScratchBufferDevAddr;

  VkAccelerationStructureCreateInfoKHR blasCreateInfo{};
  blasCreateInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  blasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  blasCreateInfo.buffer = this->_blasBuffer.getBuffer();
  blasCreateInfo.offset = 0;
  blasCreateInfo.size = blasBuildSizes.accelerationStructureSize;

  VkAccelerationStructureKHR blas;
  if (Application::vkCreateAccelerationStructureKHR(
          app.getDevice(),
          &blasCreateInfo,
          nullptr,
          &blas) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create acceleration structure!");
  }

  this->_blas.set(app.getDevice(), blas);

  blasBuildInfo.dstAccelerationStructure = this->_blas;

  this->_pBlasBuildRanges = this->_blasBuildRanges.data();

  Application::vkCmdBuildAccelerationStructuresKHR(
      commandBuffer,
      1,
      &blasBuildInfo,
      &this->_pBlasBuildRanges);

  // Add task to delete all temp buffers once the commands have completed
  commandBuffer.addPostCompletionTask(
      [device = app.getDevice(),
       pScratchBuffer = new BufferAllocation(std::move(blasScratchBuffer))]() {
        // TODO: Get rid of this
        vkDeviceWaitIdle(device);
        delete pScratchBuffer;
      });

  // Finish BLAS building before starting TLAS building
  {
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0,
        1,
        &barrier,
        0,
        nullptr,
        0,
        nullptr);
  }

  // Now build TLAS
  VkAccelerationStructureDeviceAddressInfoKHR blasDevAddrInfo{};
  blasDevAddrInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  blasDevAddrInfo.accelerationStructure = this->_blas;
  VkDeviceAddress blasBufferDevAddr =
      Application::vkGetAccelerationStructureDeviceAddressKHR(
          app.getDevice(),
          &blasDevAddrInfo);

  VkAccelerationStructureInstanceKHR tlasInstance{};
  tlasInstance.transform =
      {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
  tlasInstance.instanceCustomIndex = 0;
  tlasInstance.mask = 0xFF;
  tlasInstance.instanceShaderBindingTableRecordOffset = 0;
  tlasInstance.flags =
      VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR |
      VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
  tlasInstance.accelerationStructureReference = blasBufferDevAddr;

  VmaAllocationCreateInfo instancesAllocInfo{};
  instancesAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  instancesAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  instancesAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  this->_tlasInstances = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      sizeof(VkAccelerationStructureInstanceKHR),
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      instancesAllocInfo);

  {
    void* pData = this->_tlasInstances.mapMemory();
    memcpy(pData, &tlasInstance, sizeof(VkAccelerationStructureInstanceKHR));
    this->_tlasInstances.unmapMemory();
  }

  VkBufferDeviceAddressInfo instancesAddrInfo{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  instancesAddrInfo.buffer = this->_tlasInstances.getBuffer();
  VkDeviceAddress instancesDevAddr =
      vkGetBufferDeviceAddress(app.getDevice(), &instancesAddrInfo);

  VkAccelerationStructureGeometryKHR tlasGeometry{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  tlasGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  tlasGeometry.geometry.instances.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
  tlasGeometry.geometry.instances.data.deviceAddress = instancesDevAddr;

  VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
  tlasBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  tlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  tlasBuildInfo.pGeometries = &tlasGeometry;
  tlasBuildInfo.geometryCount = 1;

  uint32_t tlasInstCount = 1;

  VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizes{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
  Application::vkGetAccelerationStructureBuildSizesKHR(
      app.getDevice(),
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &tlasBuildInfo,
      &tlasInstCount,
      &tlasBuildSizes);

  // Create backing buffer and scratch buffer for TLAS
  // VmaAllocationCreateInfo accelStrAllocInfo{};
  // accelStrAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  this->_tlasBuffer = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      tlasBuildSizes.accelerationStructureSize,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      accelStrAllocInfo);

  BufferAllocation tlasScratchBuffer = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      tlasBuildSizes.buildScratchSize,
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      accelStrAllocInfo);

  VkBufferDeviceAddressInfo tlasScratchBufferAddrInfo{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  tlasScratchBufferAddrInfo.buffer = tlasScratchBuffer.getBuffer();
  VkDeviceAddress tlasScratchBufferDevAddr =
      vkGetBufferDeviceAddress(app.getDevice(), &tlasScratchBufferAddrInfo);

  tlasBuildInfo.scratchData.deviceAddress = tlasScratchBufferDevAddr;

  VkAccelerationStructureCreateInfoKHR tlasCreateInfo{};
  tlasCreateInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  tlasCreateInfo.buffer = this->_tlasBuffer.getBuffer();
  tlasCreateInfo.offset = 0;
  tlasCreateInfo.size = tlasBuildSizes.accelerationStructureSize;

  VkAccelerationStructureKHR tlas;
  if (Application::vkCreateAccelerationStructureKHR(
          app.getDevice(),
          &tlasCreateInfo,
          nullptr,
          &tlas) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create acceleration structure!");
  }

  this->_tlas.set(app.getDevice(), tlas);

  tlasBuildInfo.dstAccelerationStructure = this->_tlas;

  VkAccelerationStructureBuildRangeInfoKHR& tlasBuildRangeInfo =
      this->_tlasBuildRanges.emplace_back();
  tlasBuildRangeInfo.primitiveCount = 1;
  tlasBuildRangeInfo.primitiveOffset = 0;
  tlasBuildRangeInfo.firstVertex = 0;
  tlasBuildRangeInfo.transformOffset = 0;

  this->_pTlasBuildRanges = this->_tlasBuildRanges.data();

  Application::vkCmdBuildAccelerationStructuresKHR(
      commandBuffer,
      1,
      &tlasBuildInfo,
      &this->_pTlasBuildRanges);

  {
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        0,
        1,
        &barrier,
        0,
        nullptr,
        0,
        nullptr);
  }

  // Add task to delete all temp buffers once the commands have completed
  commandBuffer.addPostCompletionTask(
      [pScratchBuffer = new BufferAllocation(std::move(tlasScratchBuffer))]() {
        // TODO: Instance buffer??
        delete pScratchBuffer;
      });
}

void AccelerationStructure::AccelerationStructureDeleter::operator()(
    VkDevice device,
    VkAccelerationStructureKHR accelerationStructure) {
  Application::vkDestroyAccelerationStructureKHR(
      device,
      accelerationStructure,
      nullptr);
}
} // namespace AltheaEngine