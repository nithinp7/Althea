#include "Image.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
Image::Image(const Application& app, const ImageOptions& options)
    : _options(options),
      _accessMask(0),
      _layout(VK_IMAGE_LAYOUT_UNDEFINED),
      _stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = options.width;
  imageInfo.extent.height = options.height;
  imageInfo.extent.depth = 1;

  imageInfo.mipLevels = options.mipCount;
  imageInfo.arrayLayers = options.layerCount;
  imageInfo.format = options.format;
  imageInfo.tiling = options.tiling;
  imageInfo.initialLayout = this->_layout;
  imageInfo.usage = options.usage;
  // TODO: might need to change this when using async
  // background-compute queues
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = options.createFlags;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.flags = 0;
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  this->_image = app.getAllocator().createImage(imageInfo, allocInfo);
}

void Image::transitionLayout(
    VkCommandBuffer commandBuffer,
    VkImageLayout newLayout,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags dstStage) {

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

  barrier.image = this->_image.getImage();

  barrier.oldLayout = this->_layout;
  barrier.newLayout = newLayout;

  barrier.srcAccessMask = this->_accessMask;
  barrier.dstAccessMask = dstAccessMask;

  barrier.subresourceRange.aspectMask = this->_options.aspectMask;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = this->_options.mipCount;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = this->_options.layerCount;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  vkCmdPipelineBarrier(
      commandBuffer,
      this->_stage,
      dstStage,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);

  this->_accessMask = dstAccessMask;
  this->_layout = newLayout;
  this->_stage = dstStage;
}

void Image::copyMipFromBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    size_t srcOffset,
    uint32_t mipLevel) {

  VkBufferImageCopy region{};
  region.bufferOffset = srcOffset;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = this->_options.aspectMask;
  region.imageSubresource.mipLevel = mipLevel;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = this->_options.layerCount;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {this->_options.width, this->_options.height, 1};

  vkCmdCopyBufferToImage(
      commandBuffer,
      srcBuffer,
      this->_image.getImage(),
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);
}
} // namespace AltheaEngine