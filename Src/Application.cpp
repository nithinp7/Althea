
#include "Application.h"

#include "DefaultTextures.h"
#include "IGameInstance.h"
#include "SingleTimeCommandBuffer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace AltheaEngine {

PFN_vkCreateAccelerationStructureKHR
    Application::vkCreateAccelerationStructureKHR;
PFN_vkDestroyAccelerationStructureKHR
    Application::vkDestroyAccelerationStructureKHR;
PFN_vkGetAccelerationStructureBuildSizesKHR
    Application::vkGetAccelerationStructureBuildSizesKHR;
PFN_vkGetAccelerationStructureDeviceAddressKHR
    Application::vkGetAccelerationStructureDeviceAddressKHR;
PFN_vkCmdBuildAccelerationStructuresKHR
    Application::vkCmdBuildAccelerationStructuresKHR;
PFN_vkBuildAccelerationStructuresKHR
    Application::vkBuildAccelerationStructuresKHR;
PFN_vkCmdTraceRaysKHR Application::vkCmdTraceRaysKHR;
PFN_vkGetRayTracingShaderGroupHandlesKHR
    Application::vkGetRayTracingShaderGroupHandlesKHR;
PFN_vkCreateRayTracingPipelinesKHR Application::vkCreateRayTracingPipelinesKHR;

std::string GApplicationTitle = "";
std::string GProjectDirectory = "";
std::string GEngineDirectory = "";

InputManager* GInputManager = nullptr;
Allocator* GAllocator = nullptr;

Application::Application(
    const std::string& appTitle,
    const std::string& projectDir,
    const std::string& engineDir,
    const CreateOptions* pCreateOptions)
    : configParser(engineDir + "/Config/ConfigFile.txt") {
  GApplicationTitle = appTitle;
  GProjectDirectory = projectDir;
  GEngineDirectory = engineDir;

  CreateOptions defaultOptions{};
  if (!pCreateOptions)
    pCreateOptions = &defaultOptions;
    
   swapChainExtent.width = pCreateOptions->width;
   swapChainExtent.height = pCreateOptions->height;
   framePacer.frameRateLimit = pCreateOptions->frameRateLimit;
   for (int i = 0; i < FramePacer::FRAME_HISTORY_COUNT; ++i) {
     framePacer.frameTimingHistory[i] = 0.0;
   }
}

void Application::run() {
  initWindow();
  initVulkan();
  this->gameInstance->initGame(*this);
  this->gameInstance->createRenderState(*this);
  mainLoop();
  this->gameInstance->destroyRenderState(*this);
  this->gameInstance->shutdownGame(*this);
  cleanup();
}

void Application::notifyWindowResized() { this->framebufferResized = true; }

static void
frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
  Application* app =
      reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
  app->notifyWindowResized();
}

void Application::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(
      swapChainExtent.width,
      swapChainExtent.height,
      GApplicationTitle.c_str(),
      nullptr,
      nullptr);
  glfwSetWindowUserPointer(window, this);

  GInputManager = new InputManager(window);
  GInputManager->addKeyBinding(
      {GLFW_KEY_ESCAPE, GLFW_PRESS, 0},
      std::bind([](Application* app) { app->shouldClose = true; }, this));

  glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
}

void Application::initVulkan() {
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createCommandPool();

  this->pAllocator = std::make_unique<Allocator>(
      this->instance,
      this->device,
      this->physicalDevice);
  GAllocator = pAllocator.get();

  depthImageFormat = findDepthFormat();

  initDefaultTextures(*this);
  createCommandBuffers();
  createSyncObjects();
}

void Application::mainLoop() {
  this->lastFrameTime = glfwGetTime();

  while (!glfwWindowShouldClose(window) && !shouldClose) {
    updateFramePacer();
    glfwPollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(device);
}

void Application::updateFramePacer() {
  if (framePacer.frameRateLimit == 0)
    return;

  double targetFrameTime = 1.0 / static_cast<double>(framePacer.frameRateLimit);

  double frameTimeAvg = 0.0;
  uint32_t validHistoryCount = 0;
  for (int i = 0; i < FramePacer::FRAME_HISTORY_COUNT; ++i) {
    if (framePacer.frameTimingHistory[i] != 0.0) {
      frameTimeAvg += framePacer.frameTimingHistory[i];
      validHistoryCount++;
    }
  }
  frameTimeAvg /= static_cast<double>(validHistoryCount);
  //std::cout << "FPS: " << std::to_string(frameTimeAvg) << "\n";

  double remainingWaitTime = targetFrameTime - frameTimeAvg;
  double currentTime = glfwGetTime();
  framePacer.frameTimingHistory[currentFrame % FramePacer::FRAME_HISTORY_COUNT] = currentTime - framePacer.cpuFrameStartTime;

  // the amount that the gpu is expected to be ahead of the cpu
  while (remainingWaitTime > 0.0) {
    glfwWaitEventsTimeout(remainingWaitTime);
    double t = glfwGetTime();
    remainingWaitTime -= (t - currentTime);
    currentTime = t;
  }

  framePacer.cpuFrameStartTime = currentTime;
}

void Application::drawFrame() {
  VkCommandBuffer& commandBuffer = commandBuffers[currentFrame];
  VkSemaphore& imageAvailableSemaphore = imageAvailableSemaphores[currentFrame];
  VkSemaphore& renderFinishedSemaphore = renderFinishedSemaphores[currentFrame];
  VkFence& inFlightFence = inFlightFences[currentFrame];

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

  vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      device,
      swapChain,
      UINT64_MAX,
      imageAvailableSemaphore,
      VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swap chain image!");
  }

  double time = glfwGetTime();
  float deltaTime = static_cast<float>(time - this->lastFrameTime);
  this->lastFrameTime = time;

  FrameContext frame{time, deltaTime, currentFrame, imageIndex};

  this->deletionTasks.tick(frame);
  this->gameInstance->tick(*this, frame);

  vkResetFences(device, 1, &inFlightFence);

  vkResetCommandBuffer(commandBuffer, 0);
  recordCommandBuffer(commandBuffer, frame);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr;

  result = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to acquire swap chain image!");
  }
}

void Application::cleanup() {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(device, commandPool, nullptr);

  this->deletionTasks.flush();

  cleanupSwapChain();

  destroyDefaultTextures();

  GAllocator = nullptr;
  this->pAllocator.reset();

  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  delete GInputManager;
  GInputManager = nullptr;

  glfwDestroyWindow(window);
  glfwTerminate();
}

bool Application::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : validationLayers) {
    bool found = false;
    for (const VkLayerProperties& layer : availableLayers) {
      bool matches = true;
      for (uint32_t j = 0; j < VK_MAX_EXTENSION_NAME_SIZE; ++j) {
        if (layerName[j] == '\0') {
          break;
        }

        if (layerName[j] != layer.layerName[j]) {
          matches = false;
          break;
        }
      }

      if (matches) {
        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

void Application::createInstance() {
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("Validation layers requested, but not available");
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Althea Renderer";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  uint32_t extensionCount;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(
      nullptr,
      &extensionCount,
      extensions.data());

  std::cout << "Available extensions:\n";
  for (const auto& extension : extensions) {
    std::cout << '\t' << extension.extensionName << '\n';
  }

  for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
    bool found = false;
    const char* glfwExtension = glfwExtensions[i];
    // Are extension names guaranteed to be this fixed size?
    for (const VkExtensionProperties& extension : extensions) {
      bool matches = true;
      for (uint32_t j = 0; j < VK_MAX_EXTENSION_NAME_SIZE; ++j) {
        if (glfwExtension[j] == '\0') {
          break;
        }

        if (glfwExtension[j] != extension.extensionName[j]) {
          matches = false;
          break;
        }
      }

      if (matches) {
        found = true;
        break;
      }
    }

    if (!found) {
      std::cout << "Missing extension: " << glfwExtension << '\n';
    }
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create instance!");
  }

  // vkGetBufferDeviceAddressKHR =
  //     reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(
  //         vkGetInstanceProcAddr(instance, "vkGetBufferDeviceAddressKHR"));
  // vkCmdBuildAccelerationStructuresKHR =
  //     reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
  //         vkGetInstanceProcAddr(instance,
  //         "vkCmdBuildAccelerationStructuresKHR"));
  // vkBuildAccelerationStructuresKHR =
  //     reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(
  //         vkGetInstanceProcAddr(instance,
  //         "vkBuildAccelerationStructuresKHR"));
  // vkCreateAccelerationStructureKHR =
  //     reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
  //         vkGetInstanceProcAddr(instance,
  //         "vkCreateAccelerationStructureKHR"));
  // vkDestroyAccelerationStructureKHR =
  //     reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
  //         vkGetInstanceProcAddr(instance,
  //         "vkDestroyAccelerationStructureKHR"));
  // vkGetAccelerationStructureBuildSizesKHR =
  //     reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
  //         vkGetInstanceProcAddr(
  //             instance,
  //             "vkGetAccelerationStructureBuildSizesKHR"));
  // vkGetAccelerationStructureDeviceAddressKHR =
  //     reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
  //         vkGetInstanceProcAddr(
  //             instance,
  //             "vkGetAccelerationStructureDeviceAddressKHR"));
  // vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
  //     vkGetInstanceProcAddr(instance, "vkCmdTraceRaysKHR"));
  // vkGetRayTracingShaderGroupHandlesKHR =
  //     reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
  //         vkGetInstanceProcAddr(instance,
  //         "vkGetRayTracingShaderGroupHandlesKHR"));
  // vkCreateRayTracingPipelinesKHR =
  //     reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
  //         vkGetInstanceProcAddr(instance, "vkCreateRayTracingPipelinesKHR"));
}

void Application::createSurface() {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface!");
  }
}

bool Application::QueueFamilyIndices::isComplete() const {
  return graphicsFamily.has_value() && presentFamily.has_value();
}

Application::QueueFamilyIndices
Application::findQueueFamilies(const VkPhysicalDevice& device) const {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      device,
      &queueFamilyCount,
      queueFamilies.data());

  int i = 0;
  for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      indices.graphicsFamily = i;
      break;
    }

    ++i;
  }

  VkBool32 presentSupport = false;
  vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

  if (presentSupport) {
    indices.presentFamily = i;
  }

  return indices;
}

bool Application::checkDeviceExtensionSupport(
    const VkPhysicalDevice& device) const {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(
      device,
      nullptr,
      &extensionCount,
      nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(
      device,
      nullptr,
      &extensionCount,
      availableExtensions.data());

  std::set<std::string> requiredExtensions(
      deviceExtensions.begin(),
      deviceExtensions.end());
  for (const VkExtensionProperties& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

bool Application::isDeviceSuitable(const VkPhysicalDevice& device) const {
  QueueFamilyIndices indices = findQueueFamilies(device);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequete = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequete = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceProperties deviceProperties;

  VkPhysicalDeviceFeatures2 deviceFeatures{};
  deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

  VkPhysicalDeviceMultiviewFeatures multiviewFeatures{};
  multiviewFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;

  VkPhysicalDeviceInlineUniformBlockFeatures inlineBlockFeatures{};
  inlineBlockFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES;

  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};

  VkPhysicalDeviceRayQueryFeaturesKHR rayQueriesFeature{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};

  VkPhysicalDeviceVulkan12Features vulkan12Features{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};

  deviceFeatures.pNext = &multiviewFeatures;
  multiviewFeatures.pNext = &inlineBlockFeatures;
  inlineBlockFeatures.pNext = &accelFeature;
  accelFeature.pNext = &rtPipelineFeature;
  rtPipelineFeature.pNext = &rayQueriesFeature;
  rayQueriesFeature.pNext = &vulkan12Features;

  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

  return indices.isComplete() && extensionsSupported && swapChainAdequete &&
         deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         deviceFeatures.features.geometryShader &&
         deviceFeatures.features.samplerAnisotropy &&
         deviceFeatures.features.fillModeNonSolid &&
         deviceFeatures.features.imageCubeArray &&
         deviceFeatures.features.wideLines &&
         deviceFeatures.features.shaderFloat64 &&
         inlineBlockFeatures.inlineUniformBlock &&
         multiviewFeatures.multiview && accelFeature.accelerationStructure &&
         // accelFeature.accelerationStructureHostCommands &&
         //  accelFeature.accelerationStructureIndirectBuild && ??
         rtPipelineFeature.rayTracingPipeline && rayQueriesFeature.rayQuery &&
         vulkan12Features.bufferDeviceAddress &&
         vulkan12Features.scalarBlockLayout &&
         vulkan12Features.runtimeDescriptorArray &&
         vulkan12Features.descriptorIndexing &&
         vulkan12Features.descriptorBindingPartiallyBound &&
         vulkan12Features.descriptorBindingUpdateUnusedWhilePending;
  //  rtPipelineFeature.rayTraversalPrimitiveCulling ??
  //  rtPipelineFeature.rayTracingPipelineTraceRaysIndirect ??; // &&
  // inlineBlockFeatures.descriptorBindingInlineUniformBlockUpdateAfterBind;
}

void Application::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  for (const VkPhysicalDevice& device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("Failed to find a suitable GPU!");
  }

  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

  rayTracingProperties.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

  VkPhysicalDeviceProperties2 prop2{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  prop2.pNext = &rayTracingProperties;
  vkGetPhysicalDeviceProperties2(physicalDevice, &prop2);
}

void Application::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
      indices.graphicsFamily.value(),
      indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos.emplace_back();
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;
  deviceFeatures.geometryShader = VK_TRUE;
  deviceFeatures.fillModeNonSolid = VK_TRUE;
  deviceFeatures.imageCubeArray = VK_TRUE;
  deviceFeatures.wideLines = VK_TRUE;
  deviceFeatures.shaderFloat64 = VK_TRUE;

  VkPhysicalDeviceInlineUniformBlockFeatures inlineBlockFeatures{};
  inlineBlockFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES;
  inlineBlockFeatures.inlineUniformBlock = VK_TRUE;
  // inlineBlockFeatures.descriptorBindingInlineUniformBlockUpdateAfterBind =
  // VK_TRUE;

  VkPhysicalDeviceMultiviewFeatures multiviewFeatures{};
  multiviewFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
  multiviewFeatures.multiview = VK_TRUE;
  multiviewFeatures.pNext = &inlineBlockFeatures;

  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
  accelFeature.accelerationStructure = VK_TRUE;
  accelFeature.pNext = &multiviewFeatures;

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
  rtPipelineFeature.rayTracingPipeline = VK_TRUE;
  rtPipelineFeature.pNext = &accelFeature;

  VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeature{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
  rayQueryFeature.rayQuery = VK_TRUE;
  rayQueryFeature.pNext = &rtPipelineFeature;

  VkPhysicalDeviceVulkan12Features vulkan12Features{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  vulkan12Features.runtimeDescriptorArray = VK_TRUE;
  vulkan12Features.scalarBlockLayout = VK_TRUE;
  vulkan12Features.bufferDeviceAddress = VK_TRUE;
  vulkan12Features.descriptorIndexing = VK_TRUE;
  vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
  vulkan12Features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
  vulkan12Features.pNext = &rayQueryFeature;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  createInfo.pNext = &vulkan12Features;

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create logical device!");
  }

  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

  vkCmdBuildAccelerationStructuresKHR =
      reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
          vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
  vkBuildAccelerationStructuresKHR =
      reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(
          vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
  vkCreateAccelerationStructureKHR =
      reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
          vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
  vkDestroyAccelerationStructureKHR =
      reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
          vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
  vkGetAccelerationStructureBuildSizesKHR =
      reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
          vkGetDeviceProcAddr(
              device,
              "vkGetAccelerationStructureBuildSizesKHR"));
  vkGetAccelerationStructureDeviceAddressKHR =
      reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
          vkGetDeviceProcAddr(
              device,
              "vkGetAccelerationStructureDeviceAddressKHR"));
  vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
      vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
  vkGetRayTracingShaderGroupHandlesKHR =
      reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
          vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
  vkCreateRayTracingPipelinesKHR =
      reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
          vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
}

Application::SwapChainSupportDetails
Application::querySwapChainSupport(const VkPhysicalDevice& device) const {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      device,
      surface,
      &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device,
        surface,
        &formatCount,
        details.formats.data());
  }
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      device,
      surface,
      &presentModeCount,
      nullptr);

  if (presentModeCount) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device,
        surface,
        &presentModeCount,
        details.presentModes.data());
  }

  return details;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
  for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR Application::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) const {
  if (!this->syncFramerate) {
    for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return availablePresentMode;
      }
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities) const {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(
        actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(
        actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

void Application::createSwapChain() {
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice);
  surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  uint32_t queueFamilyIndices[] = {
      indices.graphicsFamily.value(),
      indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(
      device,
      swapChain,
      &imageCount,
      swapChainImages.data());

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;

  swapChainImageViews.resize(swapChainImages.size());
  ImageViewOptions swapChainViewOptions{};
  swapChainViewOptions.format = swapChainImageFormat;
  for (size_t i = 0; i < swapChainImages.size(); ++i) {
    swapChainImageViews[i] =
        ImageView(*this, swapChainImages[i], swapChainViewOptions);
  }
}

void Application::cleanupSwapChain() {
  this->swapChainImageViews.clear();
  vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void Application::recreateSwapChain() {
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device);

  this->gameInstance->destroyRenderState(*this);

  cleanupSwapChain();
  createSwapChain();

  this->gameInstance->createRenderState(*this);
}

VkFormat Application::findSupportedFormat(
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (properties.linearTilingFeatures & features) == features) {
      return format;
    } else if (
        tiling == VK_IMAGE_TILING_OPTIMAL &&
        (properties.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("Failed to find supported format!");
}

VkFormat Application::findDepthFormat() {
  return findSupportedFormat(
      {// VK_FORMAT_D32_SFLOAT,
       VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool Application::hasStencilComponent() const {
  return depthImageFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         depthImageFormat == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Application::createCommandPool() {
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool!");
  }
}

void Application::createCommandBuffers() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create command buffers!");
  }
}

void Application::createSyncObjects() {
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (vkCreateSemaphore(
            device,
            &semaphoreInfo,
            nullptr,
            &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(
            device,
            &semaphoreInfo,
            nullptr,
            &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
            VK_SUCCESS) {
      throw std::runtime_error("Failed to create sync objects!");
    }
  }
}

void Application::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    const FrameContext& frame) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  isRecordingCommandBuffer = true;
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("Failed to begin recording command buffer!");
  }

  this->gameInstance->draw(*this, commandBuffer, frame);

  isRecordingCommandBuffer = false;
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to record command buffer!");
  }
}

void Application::partialSubmitWaitGpu(VkCommandBuffer commandBuffer, const FrameContext& frame) {
  if (!isRecordingCommandBuffer)
  {
    throw std::runtime_error("Attempting to partial submit when command buffer is not being recorded");
  }

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to record command buffer!");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
    std::cout << "Failed to submit SingleTimeCommandBuffer";
  }

  vkQueueWaitIdle(graphicsQueue);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  vkResetCommandBuffer(commandBuffer, 0);
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("Failed to begin recording command buffer!");
  }
}
} // namespace AltheaEngine
