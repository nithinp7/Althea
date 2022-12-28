#pragma once

#include "IGameInstance.h"
#include "CameraController.h"
#include "Cubemap.h"

#include "RenderPass.h"
#include "Model.h"

using namespace AltheaEngine;

namespace AltheaEngine {
class Application;
} // namespace AltheaEngine

class SponzaTest : public IGameInstance {
public:
  SponzaTest();
  // virtual ~SponzaTest();

  virtual void initGame(Application& app) override;
  virtual void shutdownGame(Application& app) override;

  void createRenderState(Application& app) override;
  void destroyRenderState(Application& app) override;

  virtual void tick(Application& app, const FrameContext& frame) override;
  virtual void draw(
      Application& app, 
      VkCommandBuffer commandBuffer, 
      const FrameContext& frame) override;

private:
  std::unique_ptr<CameraController> _pCameraController;
  std::unique_ptr<Model> _pSponzaModel;
  std::unique_ptr<Cubemap> _pCubemap;
  std::unique_ptr<RenderPass> _pRenderPass;
};
