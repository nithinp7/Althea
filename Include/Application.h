#pragma once

#include "ConfigParser.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstdint>
#include <vector> 
#include <optional>
#include <chrono>

namespace AltheaEngine {
class RenderPassManager;
class RenderPass;

class Application {
public:
  Application();

  void run();
  void notifyWindowResized();

private:
  const uint32_t WIDTH = 1080;
  const uint32_t HEIGHT = 960;

  const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

  std::chrono::steady_clock::time_point startTime;
  std::chrono::steady_clock::time_point lastFrameTime;

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

  VkPhysicalDeviceProperties physicalDeviceProperties{};

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  VkCommandPool commandPool;

  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  
  bool framebufferResized = false;

  uint32_t currentFrame = 0;

  ConfigParser configParser;
  RenderPassManager* pRenderPassManager;
  const RenderPass* pDefaultRenderPass;

  // TODO: group imageView and frameBuffer into a class
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;

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
  void cleanupSwapChain();
  void recreateSwapChain();
  void createImageViews();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffers();
  void createSyncObjects();

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) const;

public:
  // Getters
  const VkDevice& getDevice() const {
    return device;
  }

  const VkPhysicalDevice& getPhysicalDevice() const {
    return physicalDevice;
  }

  const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() const {
    return physicalDeviceProperties;
  }

  const VkCommandPool& getCommandPool() const {
    return commandPool;
  }

  const VkExtent2D& getSwapChainExtent() const {
    return swapChainExtent;
  }

  const VkFormat& getSwapChainImageFormat() const {
    return swapChainImageFormat;
  } 

  const ConfigParser& getConfigParser() const {
    return configParser;
  }

  uint32_t getMaxFramesInFlight() const {
    return MAX_FRAMES_IN_FLIGHT;
  }

  // Command Utilities
  VkCommandBuffer beginSingleTimeCommands() const;
  void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

  // Buffer Utilities
  uint32_t findMemoryType(
      uint32_t typeFilter,
      const VkMemoryPropertyFlags& properties) const;
  void copyBuffer(
      const VkBuffer& srcBuffer,
      const VkBuffer& dstBuffer,
      VkDeviceSize size) const;
  void createBuffer(
      VkDeviceSize size, 
      VkBufferUsageFlags usage, 
      VkMemoryPropertyFlags properties, 
      VkBuffer& buffer, 
      VkDeviceMemory& bufferMemory) const;
  void createVertexBuffer(
      const void* pSrc,
      VkDeviceSize bufferSize,
      VkBuffer& vertexBuffer,
      VkDeviceMemory& vertexBufferMemory) const;
  void createIndexBuffer(
      const void* pSrc,
      VkDeviceSize bufferSize,
      VkBuffer& indexBuffer,
      VkDeviceMemory& indexBufferMemory) const;
  void createUniformBuffers(
      VkDeviceSize bufferSize,
      std::vector<VkBuffer>& uniformBuffers,
      std::vector<VkDeviceMemory>& uniformBuffersMemory) const;

  // Image Utilities
  void createTextureImage(
      void* pSrc,
      VkDeviceSize bufferSize,
      uint32_t width,
      uint32_t height,
      VkFormat format,
      VkImage& image,
      VkDeviceMemory& imageMemory) const;
  void createImage(
      uint32_t width,
      uint32_t height,
      VkFormat format,
      VkImageTiling tiling,
      VkImageUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkImage& image,
      VkDeviceMemory& imageMemory) const;
  VkImageView createImageView(
      VkImage image,
      VkFormat format) const;
  void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const;
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
};
} // namespace AltheaEngine
