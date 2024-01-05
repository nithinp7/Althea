#pragma once

#include "Allocator.h"
#include "CameraController.h"
#include "ConfigParser.h"
#include "DeletionTasks.h"
#include "FrameContext.h"
#include "Image.h"
#include "ImageView.h"
#include "InputManager.h"
#include "Library.h"

#define GLFW_INCLUDE_VULKAN
#include "vk_mem_alloc.h"

#include <GLFW/glfw3.h>
#include <gsl/span>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGltf {
struct ImageCesium;
};

namespace AltheaEngine {
class IGameInstance;

extern std::string GProjectDirectory;
extern std::string GEngineDirectory;

// TODO: Standardize the conventions in this class ALTHEA_API with the rest of
// the repository (e.g., "pFoo" for pointers, "_foo" for private members, etc.)
class ALTHEA_API Application {
public:
  static PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
  static PFN_vkDestroyAccelerationStructureKHR
      vkDestroyAccelerationStructureKHR;
  static PFN_vkGetAccelerationStructureBuildSizesKHR
      vkGetAccelerationStructureBuildSizesKHR;
  static PFN_vkGetAccelerationStructureDeviceAddressKHR
      vkGetAccelerationStructureDeviceAddressKHR;
  static PFN_vkCmdBuildAccelerationStructuresKHR
      vkCmdBuildAccelerationStructuresKHR;
  static PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
  static PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
  static PFN_vkGetRayTracingShaderGroupHandlesKHR
      vkGetRayTracingShaderGroupHandlesKHR;
  static PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

  Application(
      const std::string& projectDirectory,
      const std::string& engineDirectory);

  template <typename TGameInstance> void createGame() {
    this->gameInstance = std::make_unique<TGameInstance>();
  }

  void run();
  void notifyWindowResized();

private:
  const uint32_t WIDTH = 1080;
  const uint32_t HEIGHT = 960;

  bool syncFramerate = true;

  double lastFrameTime = 0.0f;

  const std::vector<const char*> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

  const std::vector<const char*> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_MULTIVIEW_EXTENSION_NAME,
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
      VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
      VK_KHR_RAY_QUERY_EXTENSION_NAME};

#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

  std::unique_ptr<Allocator> pAllocator;
  std::unique_ptr<IGameInstance> gameInstance;
  DeletionTasks deletionTasks;

  GLFWwindow* window;

  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
  VkSurfaceKHR surface;
  VkSurfaceFormatKHR surfaceFormat;
  VkPresentModeKHR presentMode;
  VkQueue presentQueue;

  VkPhysicalDeviceProperties physicalDeviceProperties{};
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties{};

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  std::vector<ImageView> swapChainImageViews;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  Image depthImage;
  ImageView depthImageView;
  VkFormat depthImageFormat;

  VkCommandPool commandPool;

  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  bool framebufferResized = false;

  uint32_t currentFrame = 0;

  ConfigParser configParser;

  InputManager* pInputManager;
  bool shouldClose = false;

  void initWindow();
  void initVulkan();
  void mainLoop();
  void drawFrame();
  void cleanup();

  // BOILER PLATE IMPLEMENTATIONS

  struct ALTHEA_API SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  bool checkValidationLayerSupport();
  void createInstance();
  void createSurface();
  bool checkDeviceExtensionSupport(const VkPhysicalDevice& device) const;
  bool isDeviceSuitable(const VkPhysicalDevice& device) const;
  void pickPhysicalDevice();
  void createLogicalDevice();
  SwapChainSupportDetails
  querySwapChainSupport(const VkPhysicalDevice& device) const;
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes) const;
  VkExtent2D
  chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
  VkFormat findSupportedFormat(
      const std::vector<VkFormat>& candidates,
      VkImageTiling tiling,
      VkFormatFeatureFlags features);
  VkFormat findDepthFormat();

  void createSwapChain();
  void cleanupSwapChain();
  void recreateSwapChain();
  void createDepthResource();
  void cleanupDepthResource();
  void createCommandPool();
  void createCommandBuffers();
  void createSyncObjects();

  void
  recordCommandBuffer(VkCommandBuffer commandBuffer, const FrameContext& frame);

public:
  // Getters
  VkInstance getInstance() const { return instance; }

  VkSurfaceKHR getSurface() const { return surface; }

  VkSurfaceFormatKHR getSurfaceFormat() const { return surfaceFormat; }

  VkPresentModeKHR getPresentMode() const { return presentMode; }

  GLFWwindow* getWindow() { return window; }

  const VkDevice& getDevice() const { return device; }

  const VkPhysicalDevice& getPhysicalDevice() const { return physicalDevice; }

  const Allocator& getAllocator() const { return *this->pAllocator; }

  const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() const {
    return physicalDeviceProperties;
  }

  const VkPhysicalDeviceRayTracingPipelinePropertiesKHR&
  getRayTracingProperties() const {
    return rayTracingProperties;
  }

  const VkCommandPool& getCommandPool() const { return commandPool; }

  const VkExtent2D& getSwapChainExtent() const { return swapChainExtent; }

  VkSwapchainKHR getSwapChain() const { return swapChain; }

  const VkFormat& getSwapChainImageFormat() const {
    return swapChainImageFormat;
  }

  const std::vector<ImageView>& getSwapChainImageViews() const {
    return swapChainImageViews;
  }

  VkFormat getDepthImageFormat() const { return depthImageFormat; }

  VkImageView getDepthImageView() const { return depthImageView; }

  Image& getDepthImage() { return depthImage; }

  bool hasStencilComponent() const;

  const ConfigParser& getConfigParser() const { return configParser; }

  InputManager& getInputManager() { return *pInputManager; }

  void addDeletiontask(DeletionTask&& task) {
    deletionTasks.addDeletionTask(std::move(task));
  }

  VkQueue getGraphicsQueue() const { return this->graphicsQueue; }

  struct ALTHEA_API QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const;
  };
  QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device) const;

  uint32_t getCurrentFrameRingBufferIndex() const { return this->currentFrame; }
};
} // namespace AltheaEngine
