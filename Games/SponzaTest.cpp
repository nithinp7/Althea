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

  this->_pSponzaModel = 
      std::make_unique<Model>(
        app,
        "Sponza.gltf");

  const static std::array<std::string, 6> skyboxImagePaths = {
    "../Content/Models/Skybox/right.jpg",
    "../Content/Models/Skybox/left.jpg",
    "../Content/Models/Skybox/top.jpg",
    "../Content/Models/Skybox/bottom.jpg",
    "../Content/Models/Skybox/front.jpg",
    "../Content/Models/Skybox/back.jpg"
  };

  this->_pSkybox = 
      std::make_unique<Cubemap>(
        app,
        skyboxImagePaths);

  // Init uniform buffer memory for the skybox
  app.createUniformBuffers(
      sizeof(ModelViewProjection),
      this->_skyboxUniformBuffers,
      this->_skyboxUniformBuffersMemory);

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
    
    subpassBuilder.pipelineBuilder
        .addUniformBufferBinding()
        .addTextureBinding()

        .addVertexShader(shaderManager, "Skybox.vert")
        .addFragmentShader(shaderManager, "Skybox.frag")

        .setCullMode(VK_CULL_MODE_FRONT_BIT)
        .setDepthTesting(false)

        .setPrimitiveCount(1);
  }

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

  // Assign descriptor sets for skybox uniform updates
  this->_skyboxDescriptorSets.resize(app.getMaxFramesInFlight());
  for (uint32_t i = 0; i < app.getMaxFramesInFlight(); ++i) {
    subpasses[0].getPipeline().assignDescriptorSet(this->_skyboxDescriptorSets[i])
        .bindUniformBufferDescriptor<ModelViewProjection>(this->_skyboxUniformBuffers[i])
        .bindTextureDescriptor(this->_pSkybox->getImageView(), this->_pSkybox->getSampler());
  }

  // Assign descriptor sets for regular object pass
  this->_pSponzaModel->assignDescriptorSets(
      app, 
      subpasses[1].getPipeline());
}

void SponzaTest::destroyRenderState(Application& app) {
  // TODO: actually release resources, descriptor sets, etc?
  this->_pSponzaModel.reset();
  this->_pSkybox.reset();
  this->_pRenderPass.reset();
  
  for (VkBuffer& uniformBuffer : this->_skyboxUniformBuffers) {
    vkDestroyBuffer(app.getDevice(), uniformBuffer, nullptr);
  }

  for (VkDeviceMemory& uniformBufferMemory : this->_skyboxUniformBuffersMemory) {
    vkFreeMemory(app.getDevice(), uniformBufferMemory, nullptr);
  }
}

void SponzaTest::tick(Application& app, const FrameContext& frame) {
  this->_pCameraController->tick(frame.deltaTime);
  const Camera& camera = this->_pCameraController->getCamera();

  const glm::mat4& projection = camera.getProjection();
  glm::mat4 view = camera.computeView();

  this->_pSponzaModel->updateUniforms(view, projection, frame);

  // Update skybox uniforms
  ModelViewProjection mvp{};
  mvp.model = glm::mat4(1.0f);
  mvp.view = view;
  mvp.projection = projection;

  // TODO: add templated utility function for this? 
  uint32_t currentFrame = frame.frameRingBufferIndex;
  void* data;
  vkMapMemory(
      app.getDevice(), 
      this->_skyboxUniformBuffersMemory[currentFrame], 
      0,
      sizeof(ModelViewProjection),
      0,
      &data);
  std::memcpy(data, &mvp, sizeof(ModelViewProjection));
  vkUnmapMemory(app.getDevice(), this->_skyboxUniformBuffersMemory[currentFrame]);
}

namespace {
struct DrawSkybox {
  void draw(
      const VkCommandBuffer& commandBuffer, 
      const VkPipelineLayout& pipelineLayout, 
      const FrameContext& frame) const {
    vkCmdBindDescriptorSets(
        commandBuffer, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipelineLayout, 
        0, 
        1, 
        pCurrentDescriptorSet,
        0,
        nullptr);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  }

  VkDescriptorSet* pCurrentDescriptorSet;
};
} // namespace

void SponzaTest::draw(
    Application& app, 
    VkCommandBuffer commandBuffer, 
    const FrameContext& frame) {
  this->_pRenderPass->begin(app, commandBuffer, frame)
  // Draw skybox
      .draw(DrawSkybox{&this->_skyboxDescriptorSets[frame.frameRingBufferIndex]})
      .nextSubpass()
  // Draw Sponza model
      .draw(*this->_pSponzaModel);
}
