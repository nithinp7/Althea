#include "SponzaTest.h"

#include "Application.h"
#include "Camera.h"
#include "GraphicsPipeline.h"
#include "Primitive.h"
#include "ModelViewProjection.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <cstdint>
#include <string>
#include <array>

using namespace AltheaEngine;

SponzaTest::SponzaTest() {}

void SponzaTest::initGame(Application& app) {
  const VkExtent2D& windowDims = app.getSwapChainExtent();
  this->_pCameraController = 
      std::make_unique<CameraController>(
        app.getInputManager(), 
        90.0f,
        (float)windowDims.width/(float)windowDims.height);
}

void SponzaTest::shutdownGame(Application& app) {
  this->_pCameraController.reset();
}

void SponzaTest::createRenderState(Application& app) {
  const VkExtent2D& extent = app.getSwapChainExtent();
  this->_pCameraController->getCamera().setAspectRatio(
      (float)extent.width/(float)extent.height);

  const static std::array<std::string, 6> skyboxImagePaths = {
    "../Content/Models/Skybox/right.jpg",
    "../Content/Models/Skybox/left.jpg",
    "../Content/Models/Skybox/top.jpg",
    "../Content/Models/Skybox/bottom.jpg",
    "../Content/Models/Skybox/front.jpg",
    "../Content/Models/Skybox/back.jpg"
  };

  this->_pSkybox = 
      std::make_unique<Skybox>(
        app,
        skyboxImagePaths);

  ShaderManager& shaderManager = app.getShaderManager();

  // TODO: Default color and depth-stencil clear values for attachments?
  VkClearValue colorClear;
  colorClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  VkClearValue depthClear;
  depthClear.depthStencil = {1.0f, 0};

  std::vector<Attachment> attachments = {
    {AttachmentType::Color, app.getSwapChainImageFormat(), colorClear, std::nullopt, false},
    {AttachmentType::Depth, app.getDepthImageFormat(), depthClear, app.getDepthImageView(), true}
  };

  std::vector<SubpassBuilder> subpassBuilders;

  // SKYBOX PASS
  {
    SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();
    subpassBuilder.colorAttachments.push_back(0);
    
    Skybox::buildPipeline(app, subpassBuilder.pipelineBuilder);
  }
  
  this->_pSponzaModel = 
      std::make_unique<Model>(
        app,
        "Sponza.gltf");

  // REGULAR PASS
  {
    SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();
    subpassBuilder.colorAttachments.push_back(0);
    subpassBuilder.depthAttachment = 1;
    
    uint32_t primitiveCount = 
        static_cast<uint32_t>(this->_pSponzaModel->getPrimitivesCount());

    Primitive::buildPipeline(subpassBuilder.pipelineBuilder);
    subpassBuilder.pipelineBuilder
        .addVertexShader(shaderManager, "BasicGltf.vert")
        .addFragmentShader(shaderManager, "BasicGltf.frag")
        
        .setPrimitiveCount(primitiveCount);
  }

  this->_pRenderPass = 
      std::make_unique<RenderPass>(
        app, 
        std::move(attachments), 
        std::move(subpassBuilders));

  std::vector<Subpass>& subpasses = this->_pRenderPass->getSubpasses();

  // TODO: combine descriptor assignment with construction?
  // Wait to create the objects until the corresponding pipeline is ready?
  
  // Assign descriptor sets for skybox uniform updates
  this->_pSkybox->assignDescriptorSets(app, subpasses[0].getPipeline());

  // Assign descriptor sets for regular object pass
  this->_pSponzaModel->assignDescriptorSets(
      app, 
      subpasses[1].getPipeline());
}

void SponzaTest::destroyRenderState(Application& app) {
  // TODO: actually release resources, descriptor sets, etc?
  this->_pSkybox.reset();
  this->_pSponzaModel.reset();
  this->_pRenderPass.reset();
}

void SponzaTest::tick(Application& app, const FrameContext& frame) {
  this->_pCameraController->tick(frame.deltaTime);
  const Camera& camera = this->_pCameraController->getCamera();

  const glm::mat4& projection = camera.getProjection();
  glm::mat4 view = camera.computeView();

  this->_pSkybox->updateUniforms(view, projection, frame);
  this->_pSponzaModel->updateUniforms(view, projection, frame);
}

void SponzaTest::draw(
    Application& app, 
    VkCommandBuffer commandBuffer, 
    const FrameContext& frame) {
  this->_pRenderPass->begin(app, commandBuffer, frame)
  // Draw skybox
      .draw(*this->_pSkybox)
      .nextSubpass()
  // Draw Sponza model
      .draw(*this->_pSponzaModel);
}
