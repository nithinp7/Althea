#pragma once

#include "Allocator.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"

#include <gsl/span>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

namespace AltheaEngine {
class Application;

/**
 * @brief A collection of options used to create an image resource with
 * reasonable default values.
 */
struct ALTHEA_API ImageOptions {
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

class ALTHEA_API Image {
public:
  Image() = default;

  /**
   * @brief Create an image and upload the first mip. All layers for the first
   * mip should be present. If options.mipCount > 1, the mip chain will be
   * generated on the GPU.
   *
   * @param app The application.
   * @param commandBuffer The command buffer to use for uploading the image and
   * optionally creating mipmaps.
   * @param mip0 The source buffer to upload the first mip from.
   * @param options Options for how this image should be created.
   */
  Image(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      gsl::span<const std::byte> mip0,
      const ImageOptions& options);
  Image(
    Application& app,
    VkCommandBuffer commandBuffer,
    gsl::span<const std::byte> mip0,
    const ImageOptions& options);

  /**
   * @brief Create an image.
   *
   * @param app The application.
   * @param options Options for how this image should be created.
   */
  Image(const Application& app, const ImageOptions& options);

  operator VkImage() const { return this->_image.getImage(); }

  /**
   * @brief Upload the a mip level for this GPU image, from CPU memory. Must
   * include all present layers back-to-back in memory.
   *
   * @param app A reference to the application, to allow for staging buffer
   * allocation.
   * @param commandBuffer The command buffer to use for the upload.
   * @param src The buffer view to upload the first mip from.
   * @param mipIndex The index of this mip.
   */
  void uploadMip(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      gsl::span<const std::byte> src,
      uint32_t mipIndex);
  void uploadMip(
      Application& app,
      VkCommandBuffer commandBuffer,
      gsl::span<const std::byte> src,
      uint32_t mipIndex);

  /**
   * @brief Generate mipmaps for this image. The first mip should have already
   * been uploaded before calling this function.
   *
   * @param commandBuffer The command buffer to use when blitting the mipchain.
   */
  void generateMipMaps(VkCommandBuffer commandBuffer);

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

  void clearLayout();

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

  void copyMipToBuffer(
      VkCommandBuffer commandBuffer,
      VkBuffer dstBuffer,
      size_t dstOffset,
      uint32_t mipLevel);

  const ImageOptions& getOptions() const { return this->_options; }

private:
  ImageOptions _options{};

  VkAccessFlags _accessMask;
  VkImageLayout _layout;
  VkPipelineStageFlags _stage;

  ImageAllocation _image;
};
} // namespace AltheaEngine