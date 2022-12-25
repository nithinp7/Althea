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
   * @brief Whether this attachment is used for presentation. If
   * this is true the attachment will be stored at the end of the render pass - 
   * otherwise it may be discarded. 
   */
  bool forPresentation;
};

// TODO: Generalize to non-graphics pipelines
struct SubpassBuilder {
  std::vector<VkAttachmentReference> colorAttachments;
  std::optional<VkAttachmentReference> depthAttachment;

  GraphicsPipelineBuilder graphicsPipeline;
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
  std::vector<Attachment> _attachments;
  std::vector<Subpass> _subpasses;

  VkRenderPass _renderPass;

  VkDevice _device;
};
} // namespace AltheaEngine