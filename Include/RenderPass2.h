#pragma once

#include "GraphicsPipeline.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <cstdint>

namespace AltheaEngine {
class Application;

enum class AttachmentType {
  Color,
  Depth
  //Stencil
};

struct Attachment {
  /**
   * @brief The type of attachment to create.
   */
  AttachmentType type;

  /**
   * @brief The image format of this attachment. 
   */
  VkFormat format;
  
  /**
   * @brief The clear color to use when loading this attachment.
   */
  VkClearValue clearValue;

  /**
   * @brief The image required to construct the frame buffer for 
   * this render pass. 
   * 
   * If this attachment will be used for presentation, leave this empty.
   * In that case, a frame buffer will be created for each swapchain image.
   */
  std::optional<VkImageView> frameBufferImageView = std::nullopt;

  /**
   * @brief This attachment is only used inside this render pass and the results
   * will not be used by subsequent uses of the image (e.g., depth attachment). 
   */
  bool internalUsageOnly = false;
};

// TODO: Generalize to non-graphics pipelines
struct SubpassBuilder {
  std::vector<uint32_t> colorAttachments;
  std::optional<uint32_t> depthAttachment;

  GraphicsPipelineBuilder pipelineBuilder;
};

class Subpass {
public:
  Subpass(
      const Application& app, 
      const PipelineContext& context, 
      const SubpassBuilder& builder);

private:
  GraphicsPipeline _pipeline;
};

class RenderPass2 {
public:
  RenderPass2(
      const Application& app, 
      std::vector<Attachment>&& attachments, 
      std::vector<SubpassBuilder>&& subpasses);
  ~RenderPass2();

private:
  void _createFrameBuffer(
      const VkExtent2D& extent, 
      const std::optional<VkImageView>& swapChainImageView);

  std::vector<Attachment> _attachments;
  std::vector<Subpass> _subpasses;

  VkRenderPass _renderPass;

  std::vector<VkFramebuffer> _frameBuffers;

  VkDevice _device;
};
} // namespace AltheaEngine