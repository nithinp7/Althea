#include "SponzaTest.h"

#include "Application.h"
#include "Camera.h"
#include "GraphicsPipeline.h"
#include "Primitive.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <cstdint>

using namespace AltheaEngine;

SponzaTest::SponzaTest() {}

void SponzaTest::init(Application& app) {
  const VkExtent2D& windowDims = app.getSwapChainExtent();
  this->_pCameraController = 
      std::make_unique<CameraController>(
        app.getInputManager(), 
        90.0f,
        (float)windowDims.width/(float)windowDims.height);

  this->_pSponzaModel = 
      std::make_unique<Model>(
        app,
        "Sponza.gltf");

  ShaderManager& shaderManager = app.getShaderManager();

  VkClearValue colorClear;
  colorClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  VkClearValue depthClear;
  depthClear.depthStencil = {1.0f, 0};

  std::vector<Attachment> attachments = {
    {AttachmentType::Color, app.getSwapChainImageFormat(), colorClear, std::nullopt, false},
    {AttachmentType::Depth, app.getDepthImageFormat(), depthClear, app.getDepthImageView(), true}
  };

  std::vector<SubpassBuilder> subpassBuilders;
  SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();
  subpassBuilder.colorAttachments.push_back(0);
  subpassBuilder.depthAttachment = 1;

  subpassBuilder.pipelineBuilder
      .setPrimitiveType(PrimitiveType::TRIANGLES)

      .setVertexBufferBinding<Vertex>()
      .addVertexAttribute(VertexAttributeType::VEC3, offsetof(Vertex, position))      
      .addVertexAttribute(VertexAttributeType::VEC3, offsetof(Vertex, tangent))      
      .addVertexAttribute(VertexAttributeType::VEC3, offsetof(Vertex, bitangent))      
      .addVertexAttribute(VertexAttributeType::VEC3, offsetof(Vertex, normal));

  for (uint32_t i = 0; i < MAX_UV_COORDS; ++i) {
    subpassBuilder.pipelineBuilder.addVertexAttribute(
        VertexAttributeType::VEC3, offsetof(Vertex, uvs[i])); 
  }     
  
  subpassBuilder.pipelineBuilder
      .addUniformBufferBinding()
      .addConstantsBufferBinding<PrimitiveConstants>()
      .addTextureBinding()
      .addTextureBinding()

      .addVertexShader(shaderManager, "BasicGltf.vert")
      .addFragmentShader(shaderManager, "BasicGltf.frag")

      .enableDynamicFrontFace();

  this->_pRenderPass = 
      std::make_unique<RenderPass2>(
        app, 
        std::move(attachments), 
        std::move(subpassBuilders));
}

void SponzaTest::tick(Application& app, const FrameContext& frame) {
  this->_pCameraController->tick(frame.deltaTime);
  const Camera& camera = this->_pCameraController->getCamera();

  const glm::mat4& projection = camera.getProjection();
  glm::mat4 view = camera.computeView();

  this->_pSponzaModel->updateUniforms(view, projection, frame);
}

void SponzaTest::draw(
    Application& app, 
    VkCommandBuffer commandBuffer, 
    const FrameContext& frame) {
  
}

void SponzaTest::shutdown(Application& app) {
  this->_pCameraController.reset();

  // TODO: actually release resources, descriptor sets, etc?
  this->_pSponzaModel.reset();
  this->_pRenderPass.reset();
}

void SponzaTest::notifyWindowSizeChange(uint32_t width, uint32_t height) {
  this->_pCameraController->getCamera().setAspectRatio(
      (float)width/(float)height);

  // TODO: rebuild render state, swapchain images and depth resources are
  // invalidated on window resize.
}
