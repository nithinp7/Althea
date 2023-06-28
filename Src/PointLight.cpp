#include "PointLight.h"

#include "Application.h"
#include "Primitive.h"
#include "Camera.h"

#include <glm/gtc/constants.hpp>

#include <memory>

namespace AltheaEngine {

// TODO: Refactor out into generalized storage buffer class...

PointLightCollection::PointLightCollection(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    size_t lightCount,
    bool createShadowMap,
    VkDescriptorSetLayout gltfMaterialLayout)
    : _dirty(true),
      _useShadowMaps(createShadowMap),
      _lights(),
      _buffer(
          app,
          commandBuffer,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          lightCount * sizeof(PointLight),
          app.getPhysicalDeviceProperties()
              .limits.minStorageBufferOffsetAlignment),
      _sphere(app, commandBuffer) {
  this->_lights.resize(lightCount);

  if (createShadowMap) {
    // Create shadow map resources
    // TODO: Configure shadowmap resolution
    VkExtent2D extent{256, 256};
    this->_shadowMap = RenderTargetCollection(
        app,
        commandBuffer,
        extent,
        lightCount,
        RenderTargetFlags::SceneCaptureCube |
            RenderTargetFlags::EnableDepthTarget);

    DescriptorSetLayoutBuilder shadowLayoutBuilder{};
    shadowLayoutBuilder.addUniformBufferBinding(VK_SHADER_STAGE_VERTEX_BIT);

    for (size_t i = 0; i < lightCount; ++i) {
      this->_shadowResources.emplace_back(app, shadowLayoutBuilder);
      this->_shadowUniforms.emplace_back(app, commandBuffer);
      this->_shadowResources.back().assign().bindTransientUniforms(
          this->_shadowUniforms.back());
    }

    // Setup shadow mapping render pass
    std::vector<SubpassBuilder> subpassBuilders;

    //  GLTF SUBPASS
    {
      SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();

      // Only interested in writing to a depth buffer
      subpassBuilder.depthAttachment = 0;

      Primitive::buildPipeline(subpassBuilder.pipelineBuilder);

      subpassBuilder
          .pipelineBuilder
          // Vertex shader
          .addVertexShader(GEngineDirectory + "/Shaders/ShadowMap.vert")
          .addFragmentShader(GEngineDirectory + "/Shaders/ShadowMap.frag")
          // TODO: Still need fragment shader for opacity masking?

          // Pipeline resource layouts
          .layoutBuilder
          // Shadow pass resources
          // TODO: Verify that this works, all the shadow resources should have
          // the same layout, but I don't know how layout-matching is checked /
          // enforced in Vulkan.
          .addDescriptorSet(this->_shadowResources[0].getLayout())
          // Material (per-object) resources (diffuse, normal map,
          // metallic-roughness, etc)
          .addDescriptorSet(gltfMaterialLayout);
    }

    VkClearValue depthClear;
    depthClear.depthStencil = {1.0f, 0};

    std::vector<Attachment> attachments = {Attachment{
        ATTACHMENT_FLAG_CUBEMAP | ATTACHMENT_FLAG_DEPTH,
        app.getDepthImageFormat(),
        depthClear,
        false,
        false,
        true}};

    this->_pShadowPass = std::make_unique<RenderPass>(
        app,
        extent,
        std::move(attachments),
        std::move(subpassBuilders));

    this->_shadowFrameBuffers.resize(lightCount);
    for (size_t i = 0; i < lightCount; ++i) {
      this->_shadowFrameBuffers[i] = FrameBuffer(
          app,
          *this->_pShadowPass,
          extent,
          {this->getShadowMapTargetView(i)});
    }
  }
}

void PointLightCollection::setLight(uint32_t lightId, const PointLight& light) {
  this->_lights[lightId] = light;

  this->_dirty = true;
}

void PointLightCollection::updateResource(const FrameContext& frame) {
  // if (!this->_dirty) {
  //   return;
  // }

  if (this->_useShadowMaps) {
    for (size_t i = 0; i < this->_lights.size(); ++i) {
      const PointLight& light = this->_lights[i];

      // TODO: Configure near / far plane, does it matter??
      Camera sceneCaptureCamera(90.0f, 1.0f, 0.01f, 1000.0f);

      ShadowMapUniforms shadowMapInfo{};
      shadowMapInfo.projection = sceneCaptureCamera.getProjection();
      shadowMapInfo.inverseProjection = glm::inverse(shadowMapInfo.projection);

      glm::mat4 view = sceneCaptureCamera.computeView();
      glm::mat4 invView = glm::inverse(view);

      // front back up down right left
      // X+ X- Y+ Y- Z+ Z-
      sceneCaptureCamera.setPosition(light.position);
      sceneCaptureCamera.setRotation(90.0f, 0.0f);
      shadowMapInfo.views[0] = sceneCaptureCamera.computeView();
      shadowMapInfo.inverseViews[0] = glm::inverse(shadowMapInfo.views[0]);

      sceneCaptureCamera.setRotation(-90.0f, 0.0f);
      shadowMapInfo.views[1] = sceneCaptureCamera.computeView();
      shadowMapInfo.inverseViews[1] = glm::inverse(shadowMapInfo.views[1]);

      sceneCaptureCamera.setRotation(180.0f, 90.0f);
      shadowMapInfo.views[2] = sceneCaptureCamera.computeView();
      shadowMapInfo.inverseViews[2] = glm::inverse(shadowMapInfo.views[2]);

      sceneCaptureCamera.setRotation(180.0f, -90.0f);
      shadowMapInfo.views[3] = sceneCaptureCamera.computeView();
      shadowMapInfo.inverseViews[3] = glm::inverse(shadowMapInfo.views[3]);

      sceneCaptureCamera.setRotation(180.0f, 0.0f);
      shadowMapInfo.views[4] = sceneCaptureCamera.computeView();
      shadowMapInfo.inverseViews[4] = glm::inverse(shadowMapInfo.views[4]);

      sceneCaptureCamera.setRotation(0.0f, 0.0f);
      shadowMapInfo.views[5] = sceneCaptureCamera.computeView();
      shadowMapInfo.inverseViews[5] = glm::inverse(shadowMapInfo.views[5]);

      this->_shadowUniforms[i].updateUniforms(shadowMapInfo, frame);
    }
  }

  this->_scratchBytes.resize(this->_buffer.getSize());
  std::memcpy(
      this->_scratchBytes.data(),
      this->_lights.data(),
      this->_buffer.getSize());
  this->_buffer.updateData(frame.frameRingBufferIndex, this->_scratchBytes);

  this->_dirty = false;
}

void PointLightCollection::setupPointLightMeshSubpass(
    SubpassBuilder& subpassBuilder,
    uint32_t colorAttachment,
    uint32_t depthAttachment,
    VkDescriptorSetLayout globalDescriptorSetLayout) const {
  subpassBuilder.colorAttachments.push_back(colorAttachment);
  subpassBuilder.depthAttachment = depthAttachment;

  subpassBuilder
      .pipelineBuilder
      // TODO: This is a hack to workaround incorrect winding of sphere
      // triangle indices - fix that instead of disabling backface culling
      .addVertexInputBinding<glm::vec3>(VK_VERTEX_INPUT_RATE_VERTEX)
      .addVertexAttribute(VertexAttributeType::VEC3, 0)

      // Vertex shader
      .addVertexShader(GEngineDirectory + "/Shaders/LightMesh.vert")
      // Fragment shader
      .addFragmentShader(GEngineDirectory + "/Shaders/LightMesh.frag")

      // Pipeline resource layouts
      .layoutBuilder
      // Global resources (view, projection, environment ma)
      .addDescriptorSet(globalDescriptorSetLayout);
}

void PointLightCollection::drawShadowMaps(
    Application& app,
    VkCommandBuffer commandBuffer,
    const FrameContext& frame,
    const std::vector<Model>& models) {

  // Shadow passes
  this->_shadowMap.transitionToAttachment(commandBuffer);

  for (uint32_t i = 0; i < this->_lights.size(); ++i) {
    const PointLight& light = this->_lights[i];

    ActiveRenderPass pass = this->_pShadowPass->begin(
        app,
        commandBuffer,
        frame,
        this->_shadowFrameBuffers[i]);

    VkDescriptorSet shadowDescriptorSet =
        this->_shadowResources[i].getCurrentDescriptorSet(frame);
    pass.setGlobalDescriptorSets(gsl::span(&shadowDescriptorSet, 1));

    // Draw models
    for (const Model& model : models) {
      pass.draw(model);
    }
  }

  this->_shadowMap.transitionToTexture(commandBuffer);
}

void PointLightCollection::draw(const DrawContext& context) const {
  context.bindDescriptorSets();
  context.bindVertexBuffer(this->_sphere.vertexBuffer);
  context.bindIndexBuffer(this->_sphere.indexBuffer);
  vkCmdDrawIndexed(
      context.getCommandBuffer(),
      this->_sphere.indexBuffer.getIndexCount(),
      static_cast<uint32_t>(this->_lights.size()),
      0,
      0,
      0);
}

PointLightCollection::Sphere::Sphere(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer) {
  constexpr uint32_t resolution = 50;
  constexpr float maxPitch = 0.499f * glm::pi<float>();

  auto sphereUvIndexToVertIndex = [resolution](uint32_t i, uint32_t j) {
    i = i % resolution;
    return i * resolution / 2 + j;
  };

  std::vector<glm::vec3> vertices;
  std::vector<uint32_t> indices;

  // Verts from the cylinder mapping
  uint32_t cylinderVertsCount = resolution * resolution / 2;
  // Will include cylinder mapped verts and 2 cap verts
  vertices.reserve(cylinderVertsCount + 2);
  indices.reserve(3 * resolution * resolution);
  for (uint32_t i = 0; i < resolution; ++i) {
    float theta = i * 2.0f * glm::pi<float>() / resolution;
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);

    for (uint32_t j = 0; j < resolution / 2; ++j) {
      float phi = j * 2.0f * maxPitch / (resolution / 2) - maxPitch;
      float cosPhi = cos(phi);
      float sinPhi = sin(phi);

      vertices.emplace_back(cosPhi * cosTheta, sinPhi, -cosPhi * sinTheta);

      if (j < resolution / 2 - 1) {
        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j + 1));

        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j + 1));
        indices.push_back(sphereUvIndexToVertIndex(i, j + 1));
      } else {
        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
        indices.push_back(cylinderVertsCount);
      }

      if (j == 0) {
        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(cylinderVertsCount + 1);
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
      }
    }
  }

  // Cap vertices
  vertices.emplace_back(0.0f, 1.0f, 0.0f);
  vertices.emplace_back(0.0f, -1.0f, 0.0f);

  this->indexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));
  this->vertexBuffer =
      VertexBuffer<glm::vec3>(app, commandBuffer, std::move(vertices));
}
} // namespace AltheaEngine