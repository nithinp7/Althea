#pragma once

#include "Application.h"
#include "FrameContext.h"

namespace AltheaEngine {
class IGameInstance {
public:
  virtual ~IGameInstance() {}

  virtual void init(Application& app) = 0;
  virtual void tick(Application& app, const FrameContext& frame) = 0;
  virtual void draw(
      Application& app, 
      VkCommandBuffer commandBuffer, 
      const FrameContext& frame) = 0;
  virtual void shutdown(Application& app) = 0;

  virtual void notifyWindowSizeChange(uint32_t width, uint32_t height) {}
};
} // namespace AltheaEngine