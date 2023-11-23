#pragma once

#include "Allocator.h"
#include "DynamicVertexBuffer.h"
#include "FrameContext.h"
#include "IndexBuffer.h"
#include "Library.h"
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
 * used in this class ALTHEA_API may be invalid if a DrawContext object is used
 * outside the lifetime of an ActiveRenderPass.
 */
class ALTHEA_API DrawContext {
public:
  /**
   * @brief Bind all needed descriptor sets for this draw call, including
   * global, render pass wide, subpass wide, and material resources.
   *
   * @param material The materal corresponding to the current object.
   */
  void bindDescriptorSets(const Material& material = Material()) const;

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
  // TODO: Support dynamic vertex buffer (uses offset depnding on frameContext)
  /**
   * @brief Bind this vertex buffer for drawing.
   *
   * @param vertexBuffer The vertex buffer to bind.
   */
  template <typename TVertex>
  void bindVertexBuffer(const VertexBuffer<TVertex>& vertexBuffer) const {
    // TODO: Support instance buffer as well
    const BufferAllocation& allocation = vertexBuffer.getAllocation();
    VkBuffer vkBuffer = allocation.getBuffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(this->_commandBuffer, 0, 1, &vkBuffer, &offset);
  }

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
   * @param instanceCount The number of instances to draw.
   */
  void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1) const;

  /**
   * @brief Binds the given vertex and index buffers and executes an indexed
   * draw call.
   *
   * @tparam TVertex The type of the vertices in the vertex buffer.
   * @param vertexBuffer The vertex buffer to bind and draw.
   * @param indexBuffer The index buffer to bind and draw.
   */
  template <typename TVertex>
  void drawIndexed(
      const VertexBuffer<TVertex>& vertexBuffer,
      const IndexBuffer& indexBuffer) const {
    // TODO: Expose more flexible draw commands, index offset, vertex offset,
    // instance count, etc.
    this->bindVertexBuffer(vertexBuffer);
    this->bindIndexBuffer(indexBuffer);
    this->drawIndexed(static_cast<uint32_t>(indexBuffer.getIndexCount()));
  }

  /**
   * @brief Execute a non-indexed draw call.
   *
   * @param vertexCount The number of vertices to execute in the
   * draw call.
   * @param instanceCount The number of instances to draw.
   */
  void draw(uint32_t vertexCount, uint32_t instanceCount = 1) const;

  /**
   * @brief Binds the given vertex buffer and executes a non-indexed draw call.
   *
   * @tparam TVertex The type of vertices in the vertex buffer.
   * @param vertexBuffer The number of vertices to execute in the
   * draw call.
   */
  template <typename TVertex>
  void draw(const VertexBuffer<TVertex>& vertexBuffer) const {
    this->bindVertexBuffer(vertexBuffer);
    this->draw(static_cast<uint32_t>(vertexBuffer.getVertexCount()));
  }

  const FrameContext& getFrame() const { return *this->_pFrame; }

  VkCommandBuffer getCommandBuffer() const { return this->_commandBuffer; }

private:
  friend class ActiveRenderPass;

  void
  _updatePushConstants(const void* pSrc, uint32_t size, uint32_t index) const;

  VkCommandBuffer _commandBuffer;

  // The number of global descriptor sets there are in the _descriptorSets
  // array (max 3).
  uint32_t _globalDescriptorSetCount = 0;
  // The global descriptor sets, optionally followed by a material descriptor
  // set.
  // Note this is mutable so we can temporarily add the material descriptor
  // set from a const function. It needs to be contiguous in memory to the
  // rest. This "scratch-space" usage should not be visible externally.
  mutable VkDescriptorSet _descriptorSets[4];

  const Subpass* _pCurrentSubpass;

  const FrameContext* _pFrame;
};
} // namespace AltheaEngine