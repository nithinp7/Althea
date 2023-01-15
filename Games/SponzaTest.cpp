#include "SponzaTest.h"

#include "Application.h"
#include "Camera.h"
#include "Cubemap.h"
#include "DescriptorSet.h"
#include "GraphicsPipeline.h"
#include "InputManager.h"
#include "ModelViewProjection.h"
#include "Primitive.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace AltheaEngine;

SponzaTest::SponzaTest() {}

void SponzaTest::initGame(Application& app) {
  const VkExtent2D& windowDims = app.getSwapChainExtent();
  this->_pCameraController = std::make_unique<CameraController>(
      app.getInputManager(),
      90.0f,
      (float)windowDims.width / (float)windowDims.height);

  InputManager& input = app.getInputManager();
  input.addKeyBinding(
      {GLFW_KEY_L, GLFW_PRESS, 0},
      [&adjustingLight = this->_adjustingLight, &input]() {
        adjustingLight = true;
        input.setMouseCursorHidden(false);
      });

  input.addKeyBinding(
      {GLFW_KEY_L, GLFW_RELEASE, 0},
      [&adjustingLight = this->_adjustingLight, &input]() {
        adjustingLight = false;
        input.setMouseCursorHidden(true);
      });

  input.addMousePositionCallback(
      [&adjustingLight = this->_adjustingLight,
       &lightDir = this->_lightDir](double x, double y, bool cursorHidden) {
        if (adjustingLight) {
          // TODO: consider current camera forward direction.
          float theta = glm::pi<float>() * static_cast<float>(x);
          float height = static_cast<float>(y) + 1.0f;

          lightDir = glm::normalize(glm::vec3(cos(theta), height, sin(theta)));
        }
      });
}

void SponzaTest::shutdownGame(Application& app) {
  this->_pCameraController.reset();
}

void SponzaTest::createRenderState(Application& app) {
  const VkExtent2D& extent = app.getSwapChainExtent();
  this->_pCameraController->getCamera().setAspectRatio(
      (float)extent.width / (float)extent.height);

  // TODO: Default color and depth-stencil clear values for attachments?
  VkClearValue colorClear;
  colorClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  VkClearValue depthClear;
  depthClear.depthStencil = {1.0f, 0};

  std::vector<Attachment> attachments = {
      {AttachmentType::Color,
       app.getSwapChainImageFormat(),
       colorClear,
       std::nullopt,
       false},
      {AttachmentType::Depth,
       app.getDepthImageFormat(),
       depthClear,
       app.getDepthImageView(),
       true}};

  // Global resources
  DescriptorSetLayoutBuilder globalResourceLayout;
  // Add slot for skybox cubemap.
  globalResourceLayout.addTextureBinding().addUniformBufferBinding();

  this->_pGlobalResources =
      std::make_shared<PerFrameResources>(app, globalResourceLayout);
  this->_pGlobalUniforms =
      std::make_unique<TransientUniforms<GlobalUniforms>>(app);

  std::vector<SubpassBuilder> subpassBuilders;

  // SKYBOX PASS
  {
    SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();
    subpassBuilder.colorAttachments.push_back(0);
    Skybox::buildPipeline(app, subpassBuilder.pipelineBuilder);
  }

  // REGULAR PASS
  {
    SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();
    subpassBuilder.colorAttachments.push_back(0);
    subpassBuilder.depthAttachment = 1;

    Primitive::buildPipeline(subpassBuilder.pipelineBuilder);

    subpassBuilder.pipelineBuilder.addVertexShader("GltfPBR.vert")
        .addFragmentShader("GltfPBR.frag");
  }

  this->_pRenderPass = std::make_unique<RenderPass>(
      app,
      this->_pGlobalResources,
      DescriptorSetLayoutBuilder{},
      std::move(attachments),
      std::move(subpassBuilders));

  std::vector<Subpass>& subpasses = this->_pRenderPass->getSubpasses();

  const static std::array<std::string, 6> skyboxImagePaths = {
      "../Content/Models/Skybox/right.jpg",
      "../Content/Models/Skybox/left.jpg",
      "../Content/Models/Skybox/top.jpg",
      "../Content/Models/Skybox/bottom.jpg",
      "../Content/Models/Skybox/front.jpg",
      "../Content/Models/Skybox/back.jpg"};

  this->_pSkybox = std::make_unique<Skybox>(app, skyboxImagePaths, true);

  this->_pSponzaModel = std::make_unique<Model>(
      app,
      // "Sponza/glTF/Sponza.gltf",
      "FlightHelmet/FlightHelmet.gltf",
      subpasses[1].getPipeline().getMaterialAllocator());

  // Bind the skybox cubemap as a global resource
  const std::shared_ptr<Cubemap>& pCubemap = this->_pSkybox->getCubemap();
  this->_pGlobalResources->assign()
      .bindTexture(pCubemap->getImageView(), pCubemap->getSampler())
      .bindTransientUniforms(*this->_pGlobalUniforms);
}

void SponzaTest::destroyRenderState(Application& app) {
  // TODO: actually release resources, descriptor sets, etc?
  this->_pSkybox.reset();
  this->_pSponzaModel.reset();
  this->_pRenderPass.reset();
  this->_pGlobalResources.reset();
  this->_pGlobalUniforms.reset();
}

void SponzaTest::tick(Application& app, const FrameContext& frame) {
  this->_pCameraController->tick(frame.deltaTime);
  const Camera& camera = this->_pCameraController->getCamera();

  const glm::mat4& projection = camera.getProjection();

  GlobalUniforms globalUniforms;
  globalUniforms.projection = camera.getProjection();
  globalUniforms.inverseProjection = glm::inverse(globalUniforms.projection);
  globalUniforms.view = camera.computeView();
  globalUniforms.inverseView = glm::inverse(globalUniforms.view);
  globalUniforms.lightDir = this->_lightDir;
  globalUniforms.time = static_cast<float>(frame.currentTime);

  this->_pGlobalUniforms->updateUniforms(globalUniforms, frame);
}

void SponzaTest::draw(
    Application& app,
    VkCommandBuffer commandBuffer,
    const FrameContext& frame) {
  this->_pRenderPass
      ->begin(app, commandBuffer, frame)
      // Draw skybox
      .draw(*this->_pSkybox)
      .nextSubpass()
      // Draw Sponza model
      .draw(*this->_pSponzaModel);
}
