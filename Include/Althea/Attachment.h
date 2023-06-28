#pragma once

#include "Library.h"

#include <vulkan/vulkan.h>

namespace AltheaEngine {
enum ALTHEA_API AttachmentTypeFlags {
  ATTACHMENT_FLAG_COLOR = 1,
  ATTACHMENT_FLAG_CUBEMAP = 2,
  ATTACHMENT_FLAG_DEPTH = 4,
  // Stencil
};

struct ALTHEA_API Attachment {
  /**
   * @brief The type of attachment to create.
   */
  int flags;

  /**
   * @brief The image format of this attachment.
   */
  VkFormat format;

  /**
   * @brief The clear color to use when loading this attachment.
   */
  VkClearValue clearValue;

  /**
   * @brief Whether this attachment will be used for presentation. If this is
   * the case, a different frame buffer should be created for each swapchain
   * image.
   */
  bool forPresent = true;

  /**
   * @brief Whether this attachment should be loaded or cleared at the beginning of the
   * render pass.
   */
  bool load = false;

  /**
   * @brief Whether this attachment should be stored at the end of the render pass. Otherwise,
   * this attachment is assumed to only be used inside this render pass and the results
   * will not be used by subsequent uses of the image (e.g., depth buffer).
   */
  bool store = true;
};
} // namespace AltheaEngine