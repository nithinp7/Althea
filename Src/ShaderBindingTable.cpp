#include "ShaderBindingTable.h"

#include "Application.h"
#include "RayTracingPipeline.h"
#include "SingleTimeCommandBuffer.h"

#include <stdexcept>
#include <cstdint>

static uint32_t alignUp(uint32_t size, uint32_t align) {
  return (size + (align - 1)) & ~(align - 1);
}

namespace AltheaEngine {
ShaderBindingTable::ShaderBindingTable(
    Application& app,
    VkCommandBuffer commandBuffer,
    const RayTracingPipeline& pipeline) {
  
    uint32_t baseAlignment =
        app.getRayTracingProperties().shaderGroupBaseAlignment;
    uint32_t handleAlignment =
        app.getRayTracingProperties().shaderGroupHandleAlignment;
    uint32_t handleSize = app.getRayTracingProperties().shaderGroupHandleSize;

    uint32_t handleSizeAligned = alignUp(handleSize, handleAlignment);

    this->_rgenRegion.stride = alignUp(handleSizeAligned, baseAlignment);
    this->_rgenRegion.size = this->_rgenRegion.stride;

    uint32_t missCount = 1;
    this->_missRegion.stride = handleSizeAligned;
    this->_missRegion.size =
        alignUp(missCount * handleSizeAligned, baseAlignment);

    uint32_t hitCount = 1;
    this->_hitRegion.stride = handleSizeAligned;
    this->_hitRegion.size =
        alignUp(hitCount * handleSizeAligned, baseAlignment);

    // Only have one raygen count, so total handle count is:
    uint32_t handleCount = 1 + missCount + hitCount;
    uint32_t dataSize = handleCount * handleSize;

    std::vector<uint8_t> handles(dataSize);
    if (Application::vkGetRayTracingShaderGroupHandlesKHR(
            app.getDevice(),
            pipeline,
            0,
            handleCount,
            dataSize,
            handles.data()) != VK_SUCCESS) {
      throw std::runtime_error(
          "Failed to get ray tracing shader group handles!");
    }

    // TODO: What is the call region??

    VkDeviceSize sbtSize = this->_rgenRegion.size + this->_hitRegion.size +
                           this->_missRegion.size + this->_callRegion.size;

    VmaAllocationCreateInfo sbtAllocInfo{};
    sbtAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    sbtAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    sbtAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    this->_sbt = BufferUtilities::createBuffer(
        app,
        commandBuffer,
        sbtSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
        sbtAllocInfo);
    {
      void* pMapped = this->_sbt.mapMemory();
      uint8_t* pData = reinterpret_cast<uint8_t*>(pMapped);

      uint32_t handleIdx = 0;
      auto getHandle = [&]() {
        return handles.data() + (handleIdx++) * handleSize;
      };

      // Copy raygen handle
      memcpy(pData, getHandle(), handleSize);
      pData += this->_rgenRegion.size;

      // Copy miss handles
      for (uint32_t i = 0; i < missCount; ++i) {
        memcpy(pData, getHandle(), handleSize);
        pData += this->_missRegion.stride;
      }

      // Copy hit handles
      pData = reinterpret_cast<uint8_t*>(pMapped) + this->_rgenRegion.size +
              this->_missRegion.size;
      for (uint32_t i = 0; i < hitCount; ++i) {
        memcpy(pData, getHandle(), handleSize);
        pData += this->_hitRegion.stride;
      }

      this->_sbt.unmapMemory();
    }

    VkBufferDeviceAddressInfo sbtAddrInfo{
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    sbtAddrInfo.buffer = this->_sbt.getBuffer();
    VkDeviceAddress sbtAddr =
        vkGetBufferDeviceAddress(app.getDevice(), &sbtAddrInfo);
    this->_rgenRegion.deviceAddress = sbtAddr;
    this->_missRegion.deviceAddress = sbtAddr + this->_rgenRegion.size;
    this->_hitRegion.deviceAddress =
        sbtAddr + this->_rgenRegion.size + this->_missRegion.size;
}
} // namespace AltheaEngine