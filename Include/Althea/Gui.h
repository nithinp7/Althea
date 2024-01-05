#pragma once

#include "FrameBuffer.h"
#include "FrameContext.h"
#include "Library.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <vulkan/vulkan.h>

namespace AltheaEngine {
class Application;

class ALTHEA_API Gui {
public:
  static void createRenderState(Application& app);
  static void destroyRenderState(Application& app);

  static void draw(
      Application& app,
      const FrameContext& frame,
      VkCommandBuffer commandBuffer);

private:
  static VkRenderPass _renderPass;
  static VkDescriptorPool _descriptorPool;
  static SwapChainFrameBufferCollection _frameBufferCollection;
};
} // namespace AltheaEngine