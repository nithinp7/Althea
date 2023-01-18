#pragma once

#include "Library.h"

#include "Application.h"
#include "FrameContext.h"

namespace AltheaEngine {
class ALTHEA_API IGameInstance {
public:
  virtual ~IGameInstance() {}

  virtual void initGame(Application& app) = 0;
  virtual void shutdownGame(Application& app) = 0;

  virtual void createRenderState(Application& app) = 0;
  virtual void destroyRenderState(Application& app) = 0;

  virtual void tick(Application& app, const FrameContext& frame) = 0;
  virtual void draw(
      Application& app,
      VkCommandBuffer commandBuffer,
      const FrameContext& frame) = 0;
};
} // namespace AltheaEngine