#pragma once

#include "IGameInstance.h"
#include "CameraController.h"

#include "RenderPass2.h"
#include "Model.h"

using namespace AltheaEngine;

namespace AltheaEngine {
class Application;
} // namespace AltheaEngine

class SponzaTest : public IGameInstance {
public:
  SponzaTest();
  // virtual ~SponzaTest();

  virtual void init(Application& app) override;
  virtual void tick(Application& app, const FrameContext& frame) override;
  virtual void draw(
      Application& app, 
      VkCommandBuffer commandBuffer, 
      const FrameContext& frame) override;
  virtual void shutdown(Application& app) override;

  virtual void notifyWindowSizeChange(uint32_t width, uint32_t height) override;
private:
  std::unique_ptr<CameraController> _pCameraController;
  std::unique_ptr<Model> _pSponzaModel;
  std::unique_ptr<RenderPass2> _pRenderPass;
};
