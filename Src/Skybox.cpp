#include "Skybox.h"

#include "Application.h"
#include "DescriptorSet.h"
#include "ResourcesAssignment.h"
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
  builder.materialResourceLayoutBuilder.addUniformBufferBinding();

  builder.addVertexShader(shaderManager, "Skybox.vert")
      .addFragmentShader(shaderManager, "Skybox.frag")

      .setCullMode(VK_CULL_MODE_FRONT_BIT)
      .setDepthTesting(false)

      .setMaterialPoolSize(1);
}

Skybox::Skybox(
    Application& app,
    const std::array<std::string, 6>& skyboxImagePaths,
    DescriptorSetAllocator& materialAllocator)
    : _device(app.getDevice()),
      _pMaterial(std::make_unique<Material>(app, materialAllocator)) {
  this->_pCubemap = std::make_shared<Cubemap>(app, skyboxImagePaths);
  app.createUniformBuffers(
      sizeof(SkyboxUniforms),
      this->_uniformBuffers,
      this->_uniformBuffersMemory);

  // TODO: These uniforms should be global too
  this->_pMaterial->assign()
      .bindUniformBuffer<SkyboxUniforms>(this->_uniformBuffers);
}

Skybox::~Skybox() {
  for (VkBuffer& uniformBuffer : this->_uniformBuffers) {
    vkDestroyBuffer(this->_device, uniformBuffer, nullptr);
  }

  for (VkDeviceMemory& uniformBufferMemory : this->_uniformBuffersMemory) {
    vkFreeMemory(this->_device, uniformBufferMemory, nullptr);
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

void Skybox::draw(const DrawContext& context) const {
  context.bindDescriptorSets(this->_pMaterial);
  vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
}
} // namespace AltheaEngine