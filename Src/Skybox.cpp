#include "Skybox.h"
#include "Application.h"
#include "GraphicsPipeline.h"
#include "ShaderManager.h"

#include <glm/gtc/matrix_inverse.hpp>

namespace {
struct SkyboxUniforms {
  glm::mat4 inverseView;
  glm::mat4 inverseProjection;
};
} // namespace

namespace AltheaEngine {

/*static*/
void Skybox::buildPipeline(Application& app, GraphicsPipelineBuilder& builder) {
  ShaderManager& shaderManager = app.getShaderManager();
  builder
      .addUniformBufferBinding()
      .addTextureBinding()

      .addVertexShader(shaderManager, "Skybox.vert")
      .addFragmentShader(shaderManager, "Skybox.frag")

      .setCullMode(VK_CULL_MODE_FRONT_BIT)
      .setDepthTesting(false)

      .setPrimitiveCount(1);
}

Skybox::Skybox(
    Application& app, 
    const std::array<std::string, 6>& skyboxImagePaths) :
    _device(app.getDevice()),
    _cubemap(app, skyboxImagePaths) {
  app.createUniformBuffers(
      sizeof(SkyboxUniforms),
      this->_uniformBuffers,
      this->_uniformBuffersMemory);
}

Skybox::~Skybox() {
  for (VkBuffer& uniformBuffer : this->_uniformBuffers) {
    vkDestroyBuffer(this->_device, uniformBuffer, nullptr);
  }

  for (VkDeviceMemory& uniformBufferMemory : this->_uniformBuffersMemory) {
    vkFreeMemory(this->_device, uniformBufferMemory, nullptr);
  }
}

// TODO: combine descriptor assignment with construction? Applies to Model too
void Skybox::assignDescriptorSets(
    Application& app, 
    GraphicsPipeline& pipeline) {
  this->_descriptorSets.resize(app.getMaxFramesInFlight());
  for (uint32_t i = 0; i < this->_descriptorSets.size(); ++i) {
    pipeline.assignDescriptorSet(this->_descriptorSets[i])
        .bindUniformBufferDescriptor<SkyboxUniforms>(this->_uniformBuffers[i])
        .bindTextureDescriptor(this->_cubemap.getImageView(), this->_cubemap.getSampler());
  }
}

void Skybox::updateUniforms(
    const glm::mat4& view, 
    const glm::mat4& projection, 
    const FrameContext& frame) const {
  SkyboxUniforms uniforms;
  uniforms.inverseView = glm::inverse(view);
  uniforms.inverseProjection = glm::inverse(projection);

  // TODO: implement templated abstraction for this, instead of
  // require user-implemented mem mapping + memcpy
  uint32_t currentFrame = frame.frameRingBufferIndex;
  void* data;
  vkMapMemory(
      this->_device, 
      this->_uniformBuffersMemory[currentFrame], 
      0,
      sizeof(SkyboxUniforms),
      0,
      &data);
  std::memcpy(data, &uniforms, sizeof(SkyboxUniforms));
  vkUnmapMemory(this->_device, this->_uniformBuffersMemory[currentFrame]);
}

void Skybox::draw(
    const VkCommandBuffer& commandBuffer, 
    const VkPipelineLayout& pipelineLayout, 
    const FrameContext& frame) const {
  vkCmdBindDescriptorSets(
      commandBuffer, 
      VK_PIPELINE_BIND_POINT_GRAPHICS, 
      pipelineLayout, 
      0, 
      1, 
      &this->_descriptorSets[frame.frameRingBufferIndex],
      0,
      nullptr);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}
} // namespace AltheaEngine