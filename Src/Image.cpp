#include "Image.h"

#include "Application.h"
#include "BufferUtilities.h"

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

Image::Image(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    gsl::span<const std::byte> mip0,
    const ImageOptions& options)
    : _options(options),
      _accessMask(0),
      _layout(VK_IMAGE_LAYOUT_UNDEFINED),
      _stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
  // TODO: Should the client be responsible for managing these usage
  // bits instead, even though provided API (e.g., uploadImage) won't
  // work if the wrong usage is given?
  // Maybe usage should be abstracted in an Althea enum, so the actual
  // Vk usage can be derived in the Image constructor.

  // Add extra usage flag bits for internal usages of the image.
  this->_options.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (this->_options.mipCount > 1) {
    this->_options.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  this->_image = createImage(app, options);
  this->uploadMip(app, commandBuffer, mip0, 0);

  if (this->_options.mipCount > 1) {
    this->generateMipMaps(commandBuffer);
  }
}

Image::Image(
    Application& app,
    VkCommandBuffer commandBuffer,
    gsl::span<const std::byte> mip0,
    const ImageOptions& options)
    : _options(options),
      _accessMask(0),
      _layout(VK_IMAGE_LAYOUT_UNDEFINED),
      _stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
  // TODO: Should the client be responsible for managing these usage
  // bits instead, even though provided API (e.g., uploadImage) won't
  // work if the wrong usage is given?
  // Maybe usage should be abstracted in an Althea enum, so the actual
  // Vk usage can be derived in the Image constructor.

  // Add extra usage flag bits for internal usages of the image.
  this->_options.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (this->_options.mipCount > 1) {
    this->_options.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  this->_image = createImage(app, options);
  this->uploadMip(app, commandBuffer, mip0, 0);

  if (this->_options.mipCount > 1) {
    this->generateMipMaps(commandBuffer);
  }
}

Image::Image(const Application& app, const ImageOptions& options)
    : _options(options),
      _accessMask(0),
      _layout(VK_IMAGE_LAYOUT_UNDEFINED),
      _stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
  this->_image = createImage(app, options);
}

void Image::uploadMip(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    gsl::span<const std::byte> src,
    uint32_t mipIndex) {
  VkBuffer stagingBuffer = commandBuffer.createStagingBuffer(app, src);
  this->copyMipFromBuffer(commandBuffer, stagingBuffer, 0, mipIndex);
}

void Image::uploadMip(
    Application& app,
    VkCommandBuffer commandBuffer,
    gsl::span<const std::byte> src,
    uint32_t mipIndex) {

  BufferAllocation* pStagingAllocation =
      new BufferAllocation(BufferUtilities::createStagingBuffer(app, src));

  this->copyMipFromBuffer(
      commandBuffer,
      pStagingAllocation->getBuffer(),
      0,
      mipIndex);

  // Delete staging buffer allocation once the transfer is complete
  app.addDeletiontask(
      {[pStagingAllocation]() { delete pStagingAllocation; },
       app.getCurrentFrameRingBufferIndex()});
}

void Image::generateMipMaps(VkCommandBuffer commandBuffer) {
  // TODO: Technically, image blitting may not be supported for certain formats
  // on certain hardware, should check support on the physical device

  // Will re-use this barrier to enable transfer reading
  VkImageMemoryBarrier readBarrier{};
  readBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  readBarrier.image = this->_image.getImage();
  readBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  readBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  readBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  readBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  readBarrier.subresourceRange.aspectMask = this->_options.aspectMask;
  readBarrier.subresourceRange.baseMipLevel = 0;
  readBarrier.subresourceRange.levelCount = 1;
  readBarrier.subresourceRange.baseArrayLayer = 0;
  readBarrier.subresourceRange.layerCount = this->_options.layerCount;
  readBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  readBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  uint32_t previousMipWidth = this->_options.width;
  uint32_t previousMipHeight = this->_options.height;
  for (uint32_t mipLevel = 1; mipLevel < this->_options.mipCount; ++mipLevel) {
    uint32_t mipWidth = this->_options.width >> mipLevel;
    if (mipWidth == 0) {
      mipWidth = 1;
    }

    uint32_t mipHeight = this->_options.height >> mipLevel;
    if (mipHeight == 0) {
      mipHeight = 1;
    }

    // Execute barrier to prepare the previous mip for reading
    readBarrier.subresourceRange.baseMipLevel = mipLevel - 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &readBarrier);

    // Blit the previous mip into this one.
    VkImageBlit region{};
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = this->_options.layerCount;
    region.srcSubresource.mipLevel = mipLevel - 1;
    region.srcSubresource.aspectMask = this->_options.aspectMask;
    region.srcOffsets[0] = {0, 0, 0};
    region.srcOffsets[1] = {
        static_cast<int32_t>(previousMipWidth),
        static_cast<int32_t>(previousMipHeight),
        1};

    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = this->_options.layerCount;
    region.dstSubresource.mipLevel = mipLevel;
    region.dstSubresource.aspectMask = this->_options.aspectMask;
    region.dstOffsets[0] = {0, 0, 0};
    region.dstOffsets[1] = {
        static_cast<int32_t>(mipWidth),
        static_cast<int32_t>(mipHeight),
        1};

    vkCmdBlitImage(
        commandBuffer,
        this->_image.getImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        this->_image.getImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region,
        VK_FILTER_LINEAR);

    previousMipWidth = mipWidth;
    previousMipHeight = mipHeight;
  }

  // Since all other transitions happen in bulk across all mips, we transition
  // the last mip here unnecessarily for consistency.
  readBarrier.subresourceRange.baseMipLevel = this->_options.mipCount - 1;
  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &readBarrier);

  // Update information about current layout to properly conduct
  // future transitions.
  this->_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  this->_accessMask = VK_ACCESS_TRANSFER_READ_BIT;
  this->_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
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

void Image::clearLayout() {
  _accessMask = VK_ACCESS_NONE;
  _layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
}

void Image::copyMipFromBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    size_t srcOffset,
    uint32_t mipLevel) {

  this->transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT);

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

void Image::copyMipToBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer dstBuffer,
    size_t dstOffset,
    uint32_t mipLevel) {
  this->transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_ACCESS_TRANSFER_READ_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT);

  VkBufferImageCopy region{};
  // region.stype = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;

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

  region.bufferOffset = dstOffset;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  // VkCopyImageToBufferInfo copyInfo{};
  // copyInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2;
  // copyInfo.dstBuffer = dstBuffer;
  // copyInfo.pRegions = &region;
  // copyInfo.regionCount = 1;
  // copyInfo.srcImage = this->_image.getImage();
  // copyInfo.srcImageLayout = this->_layout;

  // vkCmdCopyImageToBuffer2(commandBuffer, &copyInfo);
  vkCmdCopyImageToBuffer(
      commandBuffer,
      this->_image.getImage(),
      this->_layout,
      dstBuffer,
      1,
      &region);
}
} // namespace AltheaEngine