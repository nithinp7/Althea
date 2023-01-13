
#include "Application.h"

#include <CesiumGltf/ImageCesium.h>
#include <vulkan/vulkan.h>

#include <stdexcept>

namespace AltheaEngine {
ImageAllocation Application::createTextureImage(
    const CesiumGltf::ImageCesium& imageSrc,
    VkFormat format) const {

  uint32_t width = static_cast<uint32_t>(imageSrc.width);
  uint32_t height = static_cast<uint32_t>(imageSrc.height);
  uint32_t mipCount = static_cast<uint32_t>(imageSrc.mipPositions.size());
  if (mipCount == 0) {
    mipCount = 1;
  }

  VmaAllocationCreateInfo stagingInfo{};
  stagingInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  stagingInfo.usage = VMA_MEMORY_USAGE_AUTO;

  VkDeviceSize bufferSize = imageSrc.pixelData.size();

  BufferAllocation stagingBuffer =
      createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingInfo);

  void* data = stagingBuffer.mapMemory();
  std::memcpy((char*)data, imageSrc.pixelData.data(), bufferSize);
  stagingBuffer.unmapMemory();

  ImageAllocation imageAllocation = createImage(
      width,
      height,
      mipCount,
      1,
      format,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

  transitionImageLayout(
      imageAllocation.getImage(),
      format,
      mipCount,
      1,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  if (imageSrc.mipPositions.empty()) {
    copyBufferToImage(
        stagingBuffer.getBuffer(),
        0,
        imageAllocation.getImage(),
        width,
        height,
        0,
        1);
  } else {
    for (uint32_t i = 0; i < mipCount; ++i) {
      const CesiumGltf::ImageCesiumMipPosition& mipPos =
          imageSrc.mipPositions[i];
      uint32_t mipWidth = width >> i;
      if (mipWidth == 0) {
        mipWidth = 1;
      }

      uint32_t mipHeight = height >> i;
      if (mipHeight == 0) {
        mipHeight = 1;
      }

      copyBufferToImage(
          stagingBuffer.getBuffer(),
          mipPos.byteOffset,
          imageAllocation.getImage(),
          mipWidth,
          mipHeight,
          i,
          1);
    }
  }

  transitionImageLayout(
      imageAllocation.getImage(),
      format,
      mipCount,
      1,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  return imageAllocation;
}

ImageAllocation Application::createCubemapImage(
    const std::array<CesiumGltf::ImageCesium, 6>& imageSrc,
    VkFormat format) const {

  VkDeviceSize layerSize = imageSrc[0].pixelData.size();
  VkDeviceSize totalSize = 6 * layerSize;

  uint32_t width = static_cast<uint32_t>(imageSrc[0].width);
  uint32_t height = static_cast<uint32_t>(imageSrc[0].height);
  uint32_t mipCount = static_cast<uint32_t>(imageSrc[0].mipPositions.size());

  // Verify all cubemap images have the same size
  for (uint32_t i = 1; i < 6; ++i) {
    if (layerSize != imageSrc[i].pixelData.size() ||
        width != static_cast<uint32_t>(imageSrc[i].width) ||
        height != static_cast<uint32_t>(imageSrc[i].height) ||
        mipCount != static_cast<uint32_t>(imageSrc[i].mipPositions.size())) {
      throw std::runtime_error("Inconsistent images within cubemap");
    }
  }

  if (mipCount == 0) {
    mipCount = 1;
  }

  ImageAllocation imageAllocation = createImage(
      width,
      height,
      mipCount,
      6,
      format,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

  VmaAllocationCreateInfo stagingInfo{};
  stagingInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  stagingInfo.usage = VMA_MEMORY_USAGE_AUTO;

  BufferAllocation stagingBuffer =
      createBuffer(totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingInfo);

  void* data = stagingBuffer.mapMemory();

  if (mipCount == 1) {
    // When there is a single mip, all 6 layers will be placed back-to-back
    // in the staging buffer.
    for (uint32_t layerIndex = 0; layerIndex < 6; ++layerIndex) {
      std::memcpy(
          (std::byte*)data + layerIndex * layerSize,
          imageSrc[layerIndex].pixelData.data(),
          layerSize);
    }
  } else {
    // When there are multiple mips, the same mip levels from each layer will
    // be placed together such that the order is:
    // mip0-layer0, mip0-layer1..., mip1-layer0, mip1-layer1,..., etc
    size_t currentOffset = 0;
    for (uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex) {
      for (uint32_t layerIndex = 0; layerIndex < 6; ++layerIndex) {
        const CesiumGltf::ImageCesiumMipPosition& mipPos =
            imageSrc[layerIndex].mipPositions[mipIndex];
        std::memcpy(
            (std::byte*)data + currentOffset,
            &imageSrc[layerIndex].pixelData[mipPos.byteOffset],
            mipPos.byteSize);
        currentOffset += mipPos.byteSize;
      }
    }
  }

  stagingBuffer.unmapMemory();

  transitionImageLayout(
      imageAllocation.getImage(),
      format,
      mipCount,
      6,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  if (mipCount == 1) {
    // All layers will be back-to-back.
    copyBufferToImage(
        stagingBuffer.getBuffer(),
        0,
        imageAllocation.getImage(),
        width,
        height,
        0,
        6);
  } else {
    // Mips of the same level across the layers will be back-to-back.
    size_t currentOffset = 0;
    for (uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex) {
      // The mip positions should be consistent across all layers.
      const CesiumGltf::ImageCesiumMipPosition& mipPos =
          imageSrc[0].mipPositions[mipIndex];

      uint32_t mipWidth = width >> mipIndex;
      if (mipWidth == 0) {
        mipWidth = 1;
      }

      uint32_t mipHeight = height >> mipIndex;
      if (mipHeight == 0) {
        mipHeight = 1;
      }

      // Copy over 6 layers from this mip level together.
      copyBufferToImage(
          stagingBuffer.getBuffer(),
          currentOffset,
          imageAllocation.getImage(),
          mipWidth,
          mipHeight,
          mipIndex,
          6);
      currentOffset += mipPos.byteSize * 6;
    }
  }

  transitionImageLayout(
      imageAllocation.getImage(),
      format,
      mipCount,
      6,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  return imageAllocation;
}

// TODO: For now, all images are device-local and must be populated via
// a staging buffer. If it is useful in the future, consider adding a
// VmaAllocationCreateInfo parameter to createImage to make it more flexible.

ImageAllocation Application::createImage(
    uint32_t width,
    uint32_t height,
    uint32_t mipCount,
    uint32_t layerCount,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags createFlags) const {

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;

  imageInfo.mipLevels = mipCount;
  imageInfo.arrayLayers = layerCount;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = createFlags;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.flags = 0;
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  return this->pAllocator->createImage(imageInfo, allocInfo);
}

void Application::transitionImageLayout(
    VkImage image,
    VkFormat format,
    uint32_t mipCount,
    uint32_t layerCount,
    VkImageLayout oldLayout,
    VkImageLayout newLayout) const {
  VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mipCount;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = layerCount;

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (hasStencilComponent()) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (
      oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
      newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (
      oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    throw std::invalid_argument("Unsupported layout transition!");
  }

  vkCmdPipelineBarrier(
      commandBuffer,
      sourceStage,
      destinationStage,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);

  this->endSingleTimeCommands(commandBuffer);
}

void Application::copyBufferToImage(
    VkBuffer buffer,
    size_t bufferOffset,
    VkImage image,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevel,
    uint32_t layerCount) const {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = bufferOffset;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = mipLevel;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = layerCount;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(
      commandBuffer,
      buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  endSingleTimeCommands(commandBuffer);
}
} // namespace AltheaEngine
