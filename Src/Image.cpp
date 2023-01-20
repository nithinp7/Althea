#include "Image.h"

#include "Application.h"

#include <glm/common.hpp>

#include <stdexcept>

namespace AltheaEngine {
static ImageAllocation
createImage(const Application& app, const ImageOptions& options) {
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
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = options.usage;
  // TODO: might need to change this when using async
  // background-compute queues
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = options.createFlags;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.flags = 0;
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  return app.getAllocator().createImage(imageInfo, allocInfo);
}

Image::Image(const Application& app, const ImageOptions& options)
    : _options(options),
      _accessMask(0),
      _layout(VK_IMAGE_LAYOUT_UNDEFINED),
      _stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
  this->_image = createImage(app, options);
}

Image::Image(
    const Application& app,
    const CesiumGltf::ImageCesium& image,
    VkImageUsageFlags usageFlags)
    : _accessMask(0),
      _layout(VK_IMAGE_LAYOUT_UNDEFINED),
      _stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
  ImageOptions& options = this->_options;
  options.width = static_cast<uint32_t>(image.width);
  options.height = static_cast<uint32_t>(image.height);
  options.mipCount = static_cast<uint32_t>(image.mipPositions.size());
  if (options.mipCount == 0) {
    options.mipCount = 1;
  }

  // All ImageCesium objects that come from readImage have this format
  // TODO: handle KTX2 images and images not from readImage...
  options.format = VK_FORMAT_R8G8B8A8_UNORM;
  // TODO: is this always true
  options.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  // Add the transfer bit since we will be uploading from a staging buffer.
  options.usage = usageFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  // Upload image into staging buffer
  VmaAllocationCreateInfo stagingInfo{};
  stagingInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  stagingInfo.usage = VMA_MEMORY_USAGE_AUTO;

  VkDeviceSize bufferSize = image.pixelData.size();

  BufferAllocation stagingBuffer = app.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      stagingInfo);

  void* data = stagingBuffer.mapMemory();
  std::memcpy((char*)data, image.pixelData.data(), bufferSize);
  stagingBuffer.unmapMemory();

  // Allocate device-only memory for the image
  this->_image = createImage(app, options);

  VkCommandBuffer commandBuffer = app.beginSingleTimeCommands();

  this->transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT);

  if (options.mipCount == 1) {
    this->copyMipFromBuffer(commandBuffer, stagingBuffer.getBuffer(), 0, 0);
  } else {
    for (uint32_t mipIndex = 0; mipIndex < options.mipCount; ++mipIndex) {
      const CesiumGltf::ImageCesiumMipPosition& mipPos =
          image.mipPositions[mipIndex];
      this->copyMipFromBuffer(
          commandBuffer,
          stagingBuffer.getBuffer(),
          mipPos.byteOffset,
          mipIndex);
    }
  }

  app.endSingleTimeCommands(commandBuffer);
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
  region.imageExtent = {
      this->_options.width >> mipLevel,
      this->_options.height >> mipLevel,
      1};
  if (region.imageExtent.width == 0) {
    region.imageExtent.width = 1;
  }

  if (region.imageExtent.height == 0) {
    region.imageExtent.height = 1;
  }

  vkCmdCopyBufferToImage(
      commandBuffer,
      srcBuffer,
      this->_image.getImage(),
      this->_layout,
      1,
      &region);
}
} // namespace AltheaEngine