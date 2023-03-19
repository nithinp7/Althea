#include "Skybox.h"

#include "Application.h"
#include "GraphicsPipeline.h"
#include "ResourcesAssignment.h"

#include <glm/gtc/matrix_inverse.hpp>

namespace AltheaEngine {

/*static*/
void Skybox::buildPipeline(GraphicsPipelineBuilder& builder) {
  builder.addVertexShader(GEngineDirectory + "/Shaders/Skybox.vert")
      .addFragmentShader(GEngineDirectory + "/Shaders/Skybox.frag")

      .setCullMode(VK_CULL_MODE_FRONT_BIT)
      .setDepthTesting(false);
}

Skybox::Skybox(
    Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const std::array<std::string, 6>& skyboxImagePaths,
    bool srgb)
    : _device(app.getDevice()) {
  this->_pCubemap =
      std::make_shared<Cubemap>(app, commandBuffer, skyboxImagePaths, srgb);
}

void Skybox::draw(const DrawContext& context) const {
  context.bindDescriptorSets();
  context.draw(3);
}
} // namespace AltheaEngine