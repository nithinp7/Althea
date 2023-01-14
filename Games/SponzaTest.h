#pragma once

#include "CameraController.h"
#include "DescriptorSet.h"
#include "IGameInstance.h"
#include "Model.h"
#include "PerFrameResources.h"
#include "RenderPass.h"
#include "Skybox.h"
#include "TransientUniforms.h"

#include <glm/glm.hpp>

using namespace AltheaEngine;

namespace AltheaEngine {
class Application;
} // namespace AltheaEngine

struct GlobalUniforms {
  glm::mat4 projection;
  glm::mat4 inverseProjection;
  glm::mat4 view;
  glm::mat4 inverseView;
  glm::vec3 lightDir;
  float time;
};

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
  glm::vec3 _lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
  bool _adjustingLight = false;
  
  std::shared_ptr<PerFrameResources> _pGlobalResources;
  std::unique_ptr<TransientUniforms<GlobalUniforms>> _pGlobalUniforms;

  std::unique_ptr<CameraController> _pCameraController;

  std::unique_ptr<RenderPass> _pRenderPass;

  std::unique_ptr<Skybox> _pSkybox;
  std::unique_ptr<Model> _pSponzaModel;

  std::string _currentShader = "BasicGltf";
  bool _envMap = false;
};
