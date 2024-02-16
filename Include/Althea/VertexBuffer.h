#pragma once

#include "Allocator.h"
#include "BindlessHandle.h"
#include "BufferUtilities.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"

#include <gsl/span>
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace AltheaEngine {
class Application;
class GlobalHeap;

template <typename TVertex> class ALTHEA_API VertexBuffer {
public:
  VertexBuffer() = default;

  // TODO: Create a from-buffer constructor similar to IndexBuffer

  VertexBuffer(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      std::vector<TVertex>&& vertices)
      : _vertices(std::move(vertices)) {
    gsl::span<const std::byte> verticesView(
        reinterpret_cast<const std::byte*>(this->_vertices.data()),
        sizeof(TVertex) * this->_vertices.size());

    VkBuffer stagingBuffer =
        commandBuffer.createStagingBuffer(app, verticesView);

    VmaAllocationCreateInfo deviceAllocInfo{};
    deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    this->_allocation = BufferUtilities::createBuffer(
        app,
        verticesView.size(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        deviceAllocInfo);

    BufferUtilities::copyBuffer(
        commandBuffer,
        stagingBuffer,
        0,
        this->_allocation.getBuffer(),
        0,
        verticesView.size());
  }

  VertexBuffer(
      Application& app,
      VkCommandBuffer commandBuffer,
      std::vector<TVertex>&& vertices)
      : _vertices(std::move(vertices)) {
    gsl::span<const std::byte> verticesView(
        reinterpret_cast<const std::byte*>(this->_vertices.data()),
        sizeof(TVertex) * this->_vertices.size());

    BufferAllocation* pStagingAllocation = new BufferAllocation(
        BufferUtilities::createStagingBuffer(app, verticesView));

    VmaAllocationCreateInfo deviceAllocInfo{};
    deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    this->_allocation = BufferUtilities::createBuffer(
        app,
        verticesView.size(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        deviceAllocInfo);

    BufferUtilities::copyBuffer(
        commandBuffer,
        pStagingAllocation->getBuffer(),
        0,
        this->_allocation.getBuffer(),
        0,
        verticesView.size());

    // Delete staging buffer allocation once the transfer is complete
    app.addDeletiontask(
        {[pStagingAllocation]() { delete pStagingAllocation; },
         app.getCurrentFrameRingBufferIndex()});
  }

  void registerToHeap(GlobalHeap& heap) {
    this->_handle = heap.registerBuffer();
    heap.updateStorageBuffer(
        this->_handle,
        this->_allocation.getBuffer(),
        0,
        this->getSize());
  }

  BufferHandle getHandle() const { return _handle; }

  const std::vector<TVertex>& getVertices() const { return this->_vertices; }

  size_t getVertexCount() const { return this->_vertices.size(); }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

  size_t getSize() const { return getVertexCount() * sizeof(TVertex); }

private:
  std::vector<TVertex> _vertices;
  BufferAllocation _allocation;
  BufferHandle _handle;
};
} // namespace AltheaEngine