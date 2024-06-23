#include "AccelerationStructure.h"

#include "Application.h"
#include "GlobalHeap.h"

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

  // std::vector<VkTransformMatrixKHR> transforms;
  // transforms.reserve(primCount);
  // for (const Model& model : models) {
  //   for (const Primitive& prim : model.getPrimitives()) {
  //     glm::mat4 primTransform = prim.computeWorldTransform();

  //     VkTransformMatrixKHR& transform = transforms.emplace_back();
  //     transform.matrix[0][0] = primTransform[0][0];
  //     transform.matrix[1][0] = primTransform[0][1];
  //     transform.matrix[2][0] = primTransform[0][2];

  //     transform.matrix[0][1] = primTransform[1][0];
  //     transform.matrix[1][1] = primTransform[1][1];
  //     transform.matrix[2][1] = primTransform[1][2];

  //     transform.matrix[0][2] = primTransform[2][0];
  //     transform.matrix[1][2] = primTransform[2][1];
  //     transform.matrix[2][2] = primTransform[2][2];

  //     transform.matrix[0][3] = primTransform[3][0];
  //     transform.matrix[1][3] = primTransform[3][1];
  //     transform.matrix[2][3] = primTransform[3][2];
  //   }
  // }

  // // Upload transforms for all primitives
  // VmaAllocationCreateInfo transformBufferAllocInfo{};
  // transformBufferAllocInfo.flags =
  //     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  // transformBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  // this->_transformBuffer = BufferUtilities::createBuffer(
  //     app,
  //     sizeof(VkTransformMatrixKHR) * primCount,
  //     VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
  //         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
  //     transformBufferAllocInfo);

  // {
  //   void* data = this->_transformBuffer.mapMemory();
  //   memcpy(data, transforms.data(), primCount * sizeof(VkTransformMatrixKHR));
  //   this->_transformBuffer.unmapMemory();
  // }

  // VkBufferDeviceAddressInfo transformBufferAddrInfo{
  //     VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  // transformBufferAddrInfo.buffer = this->_transformBuffer.getBuffer();
  // VkDeviceAddress transformBufferDevAddr =
  //     vkGetBufferDeviceAddress(app.getDevice(), &transformBufferAddrInfo);

  // Each gltf primitive will be it's BLAS with one geometry, and correspond to
  // its own TLAS instance. This way, individual primitives can rigidly
  // transform without updating the BLAS. It also allows us to use
  // gl_InstanceCustomIndexEXT to find the primitive's resources in the bindless
  // heap.
  std::vector<VkAccelerationStructureInstanceKHR> instances;
  instances.reserve(primCount);

  this->_blasBuffers.reserve(primCount);

  // TODO: Since the scratch buffer gets re-used, the BLASs have to be built
  // back-to-back. It would be better to have a few different scratch buffers
  // and build several BLASs simultaneously

  // TODO: Go through and determine amount of scratch space needed before-hand

  // Will be re-used for each BLAS. If a BLAS needs more scratch space, a new buffer
  // will be added.
  uint32_t blasScratchBufferSize = 0;
  VkDeviceAddress blasScratchBufferDevAddr;
  std::vector<BufferAllocation> blasScratchBuffers;

  uint32_t primIndex = 0;

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

      VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};

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

      // geometry.geometry.triangles.transformData = {};
          // transformBufferDevAddr + primIndex * sizeof(VkTransformMatrixKHR);

      geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;

      uint32_t triCount = indexBuffer.getIndexCount() / 3;
      VkAccelerationStructureBuildRangeInfoKHR buildRange{};
      buildRange.firstVertex = 0;
      buildRange.primitiveCount = triCount;
      buildRange.primitiveOffset = 0;
      buildRange.transformOffset = 0;

      VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfo{
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
      blasBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      blasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      blasBuildInfo.pGeometries = &geometry;
      blasBuildInfo.geometryCount = 1;

      VkAccelerationStructureBuildSizesInfoKHR blasBuildSizes{
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
      Application::vkGetAccelerationStructureBuildSizesKHR(
          app.getDevice(),
          VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
          &blasBuildInfo,
          &triCount,
          &blasBuildSizes);

      // Create backing buffer and scratch buffer for BLAS
      VmaAllocationCreateInfo accelStrAllocInfo{};
      accelStrAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

      this->_blasBuffers.emplace_back(BufferUtilities::createBuffer(
          app,
          blasBuildSizes.accelerationStructureSize,
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          accelStrAllocInfo));

      // Add a new scratch buffer if this one is too small
      if (blasBuildSizes.buildScratchSize > blasScratchBufferSize) {
        blasScratchBufferSize = blasBuildSizes.buildScratchSize;

        blasScratchBuffers.emplace_back(BufferUtilities::createBuffer(
            app,
            blasScratchBufferSize,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            accelStrAllocInfo));

        VkBufferDeviceAddressInfo blasScratchBufferAddrInfo{
            VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        blasScratchBufferAddrInfo.buffer = blasScratchBuffers.back().getBuffer();

        blasScratchBufferDevAddr = vkGetBufferDeviceAddress(
            app.getDevice(),
            &blasScratchBufferAddrInfo);
      }

      blasBuildInfo.scratchData.deviceAddress = blasScratchBufferDevAddr;

      VkAccelerationStructureCreateInfoKHR blasCreateInfo{};
      blasCreateInfo.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
      blasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      blasCreateInfo.buffer = this->_blasBuffers.back().getBuffer();
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

      this->_blasCollection.emplace_back(app.getDevice(), blas);

      blasBuildInfo.dstAccelerationStructure = blas;

      VkAccelerationStructureBuildRangeInfoKHR* pBlasBuildRange = &buildRange; // ???

      Application::vkCmdBuildAccelerationStructuresKHR(
          commandBuffer,
          1,
          &blasBuildInfo,
          &pBlasBuildRange/*????*/);

      VkAccelerationStructureDeviceAddressInfoKHR blasDevAddrInfo{};
      blasDevAddrInfo.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
      blasDevAddrInfo.accelerationStructure = blas;
      VkDeviceAddress blasBufferDevAddr =
          Application::vkGetAccelerationStructureDeviceAddressKHR(
              app.getDevice(),
              &blasDevAddrInfo);

      VkAccelerationStructureInstanceKHR& tlasInstance =
          instances.emplace_back();
      glm::mat4 primTransform = prim.getTransform();

      tlasInstance.transform.matrix[0][0] = primTransform[0][0];
      tlasInstance.transform.matrix[1][0] = primTransform[0][1];
      tlasInstance.transform.matrix[2][0] = primTransform[0][2];

      tlasInstance.transform.matrix[0][1] = primTransform[1][0];
      tlasInstance.transform.matrix[1][1] = primTransform[1][1];
      tlasInstance.transform.matrix[2][1] = primTransform[1][2];

      tlasInstance.transform.matrix[0][2] = primTransform[2][0];
      tlasInstance.transform.matrix[1][2] = primTransform[2][1];
      tlasInstance.transform.matrix[2][2] = primTransform[2][2];

      tlasInstance.transform.matrix[0][3] = primTransform[3][0];
      tlasInstance.transform.matrix[1][3] = primTransform[3][1];
      tlasInstance.transform.matrix[2][3] = primTransform[3][2];

      tlasInstance.instanceCustomIndex = primIndex;
      tlasInstance.mask = 0xFF;
      tlasInstance.instanceShaderBindingTableRecordOffset =
          0; // For now everything uses same shader
      tlasInstance.flags =
          VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR |
          VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
      tlasInstance.accelerationStructureReference = blasBufferDevAddr;

      ++primIndex;

      // Barrier to protect re-used scratch buffer between BLAS builds
      if (primIndex < primCount) {
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
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
    }
  }

  // Add task to delete all temp buffers once the commands have completed
  commandBuffer.addPostCompletionTask(
      [pScratchBuffers = new std::vector<BufferAllocation>(std::move(blasScratchBuffers))]() {
        delete pScratchBuffers;
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
  VmaAllocationCreateInfo instancesAllocInfo{};
  instancesAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  instancesAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  instancesAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  this->_tlasInstances = BufferUtilities::createBuffer(
      app,
      sizeof(VkAccelerationStructureInstanceKHR),
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      instancesAllocInfo);

  {
    void* pData = this->_tlasInstances.mapMemory();
    memcpy(pData, instances.data(), sizeof(VkAccelerationStructureInstanceKHR) * instances.size());
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

  uint32_t tlasInstCount = static_cast<uint32_t>(instances.size());

  VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizes{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
  Application::vkGetAccelerationStructureBuildSizesKHR(
      app.getDevice(),
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &tlasBuildInfo,
      &tlasInstCount,
      &tlasBuildSizes);

  // Create backing buffer and scratch buffer for TLAS
  VmaAllocationCreateInfo accelStrAllocInfo{};
  accelStrAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  this->_tlasBuffer = BufferUtilities::createBuffer(
      app,
      tlasBuildSizes.accelerationStructureSize,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      accelStrAllocInfo);

  BufferAllocation tlasScratchBuffer = BufferUtilities::createBuffer(
      app,
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

  VkAccelerationStructureBuildRangeInfoKHR tlasBuildRangeInfo{};
  tlasBuildRangeInfo.primitiveCount = tlasInstCount;
  tlasBuildRangeInfo.primitiveOffset = 0;
  tlasBuildRangeInfo.firstVertex = 0;
  tlasBuildRangeInfo.transformOffset = 0;

  VkAccelerationStructureBuildRangeInfoKHR* pTlasBuildRange = &tlasBuildRangeInfo;

  Application::vkCmdBuildAccelerationStructuresKHR(
      commandBuffer,
      1,
      &tlasBuildInfo,
      &pTlasBuildRange);

  // Add task to delete all temp buffers once the commands have completed
  commandBuffer.addPostCompletionTask(
      [pScratchBuffer = new BufferAllocation(std::move(tlasScratchBuffer))]() {
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

void AccelerationStructure::registerToHeap(GlobalHeap& heap) {
  _tlasHandle = heap.registerTlas();
  heap.updateTlas(_tlasHandle, _tlas);
}
} // namespace AltheaEngine