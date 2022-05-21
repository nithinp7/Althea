#pragma once

#include "RenderPass.h"
#include "ConfigParser.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstdint>
#include <vector> 
#include <optional>

class Application {
public:
  Application();

  void run();

private:
  const uint32_t WIDTH = 1080;
  const uint32_t HEIGHT = 960;

  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

  GLFWwindow* window;

  // TODO: refactor this into OO-heirarchy!!!
  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
  VkSurfaceKHR surface;
  VkQueue presentQueue;

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;

  ConfigParser configParser;
  RenderPassManager* pRenderPassManager;
  const RenderPass* pDefaultRenderPass;

  // TODO: group imageView and frameBuffer into a class
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;

  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;

  void initWindow();
  void initVulkan();
  void mainLoop();
  void drawFrame();
  void cleanup();

  // BOILER PLATE IMPLEMENTATIONS
  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const;
  };
  
  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  bool checkValidationLayerSupport();
  void createInstance();
  void createSurface();
  QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device) const;
  bool checkDeviceExtensionSupport(const VkPhysicalDevice& device) const;
  bool isDeviceSuitable(const VkPhysicalDevice& device) const;
  void pickPhysicalDevice();
  void createLogicalDevice();
  SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device) const;
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
  void createSwapChain();
  void createImageViews();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffer();
  void createSyncObjects();

  void recordCommandBuffer(VkCommandBuffer commandBuffer_, uint32_t imageIndex);
};