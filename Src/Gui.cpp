#include "Gui.h"

#include "Application.h"
#include "SingleTimeCommandBuffer.h"

#include <vulkan/vulkan.h>

#include <stdexcept>

namespace AltheaEngine {

VkRenderPass Gui::_renderPass = VK_NULL_HANDLE;
VkDescriptorPool Gui::_descriptorPool = VK_NULL_HANDLE;
SwapChainFrameBufferCollection Gui::_frameBufferCollection = {};

// From https://vkguide.dev/docs/extra-chapter/implementing_imgui/
// TODO: Would be nice to better adapt this into the engine, maybe use
// bindless instead etc..

/*static*/
void Gui::createRenderState(Application& app) {
  uint32_t swapChainImageCount = app.getSwapChainImageViews().size();

  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 10},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 10;
  pool_info.poolSizeCount = std::size(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  if (vkCreateDescriptorPool(
          app.getDevice(),
          &pool_info,
          nullptr,
          &_descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set pool for imgui");
  }

  ImGui::CreateContext();

  ImGui_ImplGlfw_InitForVulkan(app.getWindow(), true);

  {
    VkAttachmentDescription attachment = {};
    attachment.format = app.getSurfaceFormat().format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;//
    attachment.loadOp =
        VK_ATTACHMENT_LOAD_OP_CLEAR; // VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;
    if (vkCreateRenderPass(app.getDevice(), &info, nullptr, &_renderPass) !=
        VK_SUCCESS)
      throw std::runtime_error("Failed to create gui render pass");
  }

  // this initializes imgui for Vulkan
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = app.getInstance();
  init_info.PhysicalDevice = app.getPhysicalDevice();
  init_info.Device = app.getDevice();
  init_info.Queue = app.getGraphicsQueue();
  init_info.DescriptorPool = _descriptorPool;
  init_info.MinImageCount = swapChainImageCount;
  init_info.ImageCount = swapChainImageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  ImGui_ImplVulkan_Init(&init_info, _renderPass);

  // Create swap chain frame buffers
  _frameBufferCollection = SwapChainFrameBufferCollection(app, _renderPass, {});
}

/*static*/
void Gui::destroyRenderState(Application& app) {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  vkDestroyDescriptorPool(app.getDevice(), _descriptorPool, nullptr);
  vkDestroyRenderPass(app.getDevice(), _renderPass, nullptr);
  _frameBufferCollection = {};
}

/*static*/
void Gui::draw(
    Application& app,
    const FrameContext& frame,
    VkCommandBuffer commandBuffer) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  static bool show = true; // ??
  ImGui::ShowDemoWindow(&show);

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  ImGui::Render();
  // ImDrawData* draw_data = ImGui::GetDrawData();
  // const bool is_minimized =
  //     (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <=
  //     0.0f);
  // if (!is_minimized) {
  //   _window.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
  //   _window.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
  //   _window.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
  //   _window.ClearValue.color.float32[3] = clear_color.w;

  //   // ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
  //   _frameRenderHelper(app, commandBuffer);

  //   // FramePresent(wd); // ??
  // }

  ImDrawData* draw_data = ImGui::GetDrawData();

  {
    VkClearValue colorClear;
    colorClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = _renderPass;
    info.framebuffer = _frameBufferCollection.getCurrentFrameBuffer(frame);
    info.renderArea.extent = app.getSwapChainExtent();
    info.clearValueCount = 1;
    info.pClearValues = &colorClear;
    vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);

  // Submit command buffer
  vkCmdEndRenderPass(commandBuffer);
}

} // namespace AltheaEngine