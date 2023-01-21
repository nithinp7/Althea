#include "SingleTimeCommandBuffer.h"

#include "Application.h"
#include "BufferUtilities.h"

namespace AltheaEngine {
SingleTimeCommandBuffer::SingleTimeCommandBuffer(const Application& app)
    : _device(app.getDevice()),
      _queue(app.getGraphicsQueue()),
      _commandPool(app.getCommandPool()) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = this->_commandPool;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(this->_device, &allocInfo, &this->_commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(this->_commandBuffer, &beginInfo);
}

SingleTimeCommandBuffer::~SingleTimeCommandBuffer() {
  vkEndCommandBuffer(this->_commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &this->_commandBuffer;

  vkQueueSubmit(this->_queue, 1, &submitInfo, VK_NULL_HANDLE);

  vkQueueWaitIdle(this->_queue);

  vkFreeCommandBuffers(
      this->_device,
      this->_commandPool,
      1,
      &this->_commandBuffer);

  // The staging buffers now get released safely.
}

VkBuffer SingleTimeCommandBuffer::createStagingBuffer(
    const Application& app,
    gsl::span<const std::byte> buffer) {
  this->_stagingBuffers.push_back(BufferUtilities::createStagingBuffer(
      app,
      this->_commandBuffer,
      buffer));
  return this->_stagingBuffers.back().getBuffer();
}
} // namespace AltheaEngine