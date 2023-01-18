#pragma once

#include "Allocator.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {
class Application;

/**
 * @brief A collection of options used to create an image resource with
 * reasonable default values.
 */
struct ImageOptions {
  uint32_t width = 1;
  uint32_t height = 1;
  uint32_t mipCount = 1;
  uint32_t layerCount = 1;
  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkImageUsageFlags usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VkImageCreateFlags createFlags = 0;
};

class Image {
public:
  Image() = default;
  Image(const Application& app, const ImageOptions& options);

  VkImage getImage() const { return this->_image.getImage(); }

  // For now, only possible to transition all mips and layers together
  // TODO: is this reasonable?
  // TODO: wrap this with another, simpler transition function that can infer
  // some args?? E.g., "transitionToComputeInput" "transitionToComputeOutput"
  // "transitionToCompute(bool read, bool write)" "transitionToFragmentInput"
  void transitionLayout(
      VkCommandBuffer commandBuffer,
      VkImageLayout newLayout,
      VkAccessFlags dstAccessMask,
      VkPipelineStageFlags dstStage);

  /**
   * @brief Populate all the layers at the specified mip level in the image from
   * the given source buffer.
   *
   * @param commandBuffer The command buffer to issue the copy command on.
   * @param srcBuffer The device-visible buffer to copy the mip data from.
   * @param srcOffset The offset within the source buffer to start copying from.
   * @param mipLevel The mip level that should be populated.
   */
  void copyMipFromBuffer(
      VkCommandBuffer commandBuffer,
      VkBuffer srcBuffer,
      size_t srcOffset,
      uint32_t mipLevel);

private:
  ImageOptions _options;

  VkAccessFlags _accessMask;
  VkImageLayout _layout;
  VkPipelineStageFlags _stage;

  ImageAllocation _image;
};
} // namespace AltheaEngine