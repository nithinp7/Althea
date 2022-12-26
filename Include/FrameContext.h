#pragma once

#include <cstdint>

namespace AltheaEngine {
struct FrameContext {
  /**
   * @brief The time in seconds that the application has been running. 
   */
  double currentTime;
  
  /**
   * @brief The time in seconds since the last frame.
   */
  float deltaTime;

  /**
   * @brief The index of the frame resources (within ring buffers) to use 
   * for this frame.
   */
  uint32_t frameRingBufferIndex;
  
  /**
   * @brief The index of the swapchain image chosen for rendering this frame. 
   */
  uint32_t swapChainImageIndex;
};
} // namespace AltheaEngine