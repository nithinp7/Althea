#pragma once

#include "FrameContext.h"
#include "IndexBuffer.h"
#include "Material.h"
#include "VertexBuffer.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
#include <stdexcept>

namespace AltheaEngine {
class Subpass;

/**
 * @brief A transient context object that helps setup and execute draw calls.
 *
 * This object should only ever be managed by an ActiveRenderPass. References
 * used in this class may be invalid if a DrawContext object is used outside the
 * lifetime of an ActiveRenderPass.
 */
class DrawContext {
public:
  /**
   * @brief Bind all needed descriptor sets for this draw call, including
   * global, render pass wide, subpass wide, and material resources.
   *
   * @param pMaterial The materal corresponding to the current object.
   */
  void bindDescriptorSets(const std::unique_ptr<Material>& pMaterial) const;

  /**
   * @brief Update a push constant range by index. This range must have been
   * distinctly added at the given index during pipeline creation.
   *
   * @tparam TPushConstants The push constants type - must match in size with
   * the corresponding push constants range in the pipeline layout at the given
   * index.
   * @param pushConstants The push constants to update the command buffer with.
   * @param index The index of this push constant range, within the pipeline
   * layout.
   */
  template <typename TPushConstants>
  void updatePushConstants(const TPushConstants& pushConstants, uint32_t index)
      const {
    this->_updatePushConstants(&pushConstants, sizeof(TPushConstants), index);
  }

  /**
   * @brief Set the front face mode for this draw call. This feature is only
   * available when it was enabled in the pipeline creation.
   *
   * @param frontFace The front face mode to set for this draw call.
   */
  void setFrontFaceDynamic(VkFrontFace frontFace) const;

  // TODO: Support multiple vertex buffers being bound together
  // e.g., instance buffer
  /**
   * @brief Bind this vertex buffer for drawing.
   *
   * @param vertexBuffer The vertex buffer to bind.
   */
  void bindVertexBuffer(const VertexBuffer& vertexBuffer) const;

  /**
   * @brief Bind this index buffer for drawing.
   *
   * @param indexBuffer The index buffer to bind.
   */
  void bindIndexBuffer(const IndexBuffer& indexBuffer) const;

  /**
   * @brief Execute an indexed draw call.
   *
   * @param indexCount The number of indices in the indexed draw.
   */
  void drawIndexed(uint32_t indexCount) const;

  /**
   * @brief Execute a non-indexed draw call.
   *
   * @param vertexCount The number of vertices to execute in the
   * draw call.
   */
  void draw(uint32_t vertexCount) const;

private:
  friend class ActiveRenderPass;

  void
  _updatePushConstants(const void* pSrc, uint32_t size, uint32_t index) const;

  VkCommandBuffer _commandBuffer;
  std::optional<VkDescriptorSet> _globalResources;
  std::optional<VkDescriptorSet> _renderPassResources;
  std::optional<VkDescriptorSet> _subpassResource;

  const Subpass* _pCurrentSubpass;

  const FrameContext* _pFrame;
};
} // namespace AltheaEngine