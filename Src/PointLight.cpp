#include "PointLight.h"

#include "Application.h"
#include "Camera.h"
#include "Primitive.h"

#include <glm/gtc/constants.hpp>

#include <memory>

namespace AltheaEngine {

namespace {
struct ShadowMapPushConstants {
  glm::mat4 model; // TODO: This should come from some sort of model buffer...
  uint32_t primitiveIdx;
  uint32_t constantsHandle;
  uint32_t pointLightsHandle;
};
} // namespace

// TODO: Refactor out into generalized storage buffer class...

PointLightCollection::PointLightCollection(
    Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap,
    size_t lightCount,
    bool createShadowMap,
    BufferHandle primConstants)
    : _dirty(true),
      _useShadowMaps(createShadowMap),
      _lights(),
      _buffer(
          app,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          lightCount * sizeof(PointLight),
          app.getPhysicalDeviceProperties()
              .limits.minStorageBufferOffsetAlignment),
      _sphere(app, commandBuffer) {

  this->_buffer.registerToHeap(heap);
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

    this->_pointLightConstants = StructuredBuffer<PointLightConstants>(app, 1);
    this->_pointLightConstants.registerToHeap(heap);

    {
      // TODO: Configure near / far plane, does it matter??
      Camera sceneCaptureCamera(90.0f, 1.0f, 0.01f, 1000.0f);

      PointLightConstants pointLightConstants{};
      pointLightConstants.projection = sceneCaptureCamera.getProjection();
      pointLightConstants.inverseProjection =
          glm::inverse(pointLightConstants.projection);

      // Cameras will be set at the origin and face along the 6 axes
      // Later we can use these matrices along with the light positions
      // to correctly draw the shadow maps.
      // TODO: Might not be super sensible to store these views in a buffer and
      // fetch them during the shadow mapping pass - might be better to generate
      // these transforms dynamically in the shader.

      // front back up down right left
      // X+ X- Y+ Y- Z+ Z-
      sceneCaptureCamera.setPosition(glm::vec3(0.0f));
      sceneCaptureCamera.setRotation(90.0f, 0.0f);
      pointLightConstants.views[0] = sceneCaptureCamera.computeView();
      pointLightConstants.inverseViews[0] =
          glm::inverse(pointLightConstants.views[0]);

      sceneCaptureCamera.setRotation(-90.0f, 0.0f);
      pointLightConstants.views[1] = sceneCaptureCamera.computeView();
      pointLightConstants.inverseViews[1] =
          glm::inverse(pointLightConstants.views[1]);

      sceneCaptureCamera.setRotation(180.0f, 90.0f);
      pointLightConstants.views[2] = sceneCaptureCamera.computeView();
      pointLightConstants.inverseViews[2] =
          glm::inverse(pointLightConstants.views[2]);

      sceneCaptureCamera.setRotation(180.0f, -90.0f);
      pointLightConstants.views[3] = sceneCaptureCamera.computeView();
      pointLightConstants.inverseViews[3] =
          glm::inverse(pointLightConstants.views[3]);

      sceneCaptureCamera.setRotation(180.0f, 0.0f);
      pointLightConstants.views[4] = sceneCaptureCamera.computeView();
      pointLightConstants.inverseViews[4] =
          glm::inverse(pointLightConstants.views[4]);

      sceneCaptureCamera.setRotation(0.0f, 0.0f);
      pointLightConstants.views[5] = sceneCaptureCamera.computeView();
      pointLightConstants.inverseViews[5] =
          glm::inverse(pointLightConstants.views[5]);

      this->_pointLightConstants.setElement(pointLightConstants, 0);
      this->_pointLightConstants.upload(app, commandBuffer);
    }

    // Setup shadow mapping render pass
    std::vector<SubpassBuilder> subpassBuilders;

    //  GLTF SUBPASS
    {
      SubpassBuilder& subpassBuilder = subpassBuilders.emplace_back();

      // Only interested in writing to a depth buffer
      subpassBuilder.depthAttachment = 0;

      Primitive::buildPipeline(subpassBuilder.pipelineBuilder);

      ShaderDefines defs{};
      defs.emplace("BINDLESS_SET", "0");

      subpassBuilder
          .pipelineBuilder
          // Vertex shader
          .addVertexShader(
              GEngineDirectory + "/Shaders/ShadowMapBindless.vert",
              defs)
          .addFragmentShader(
              GEngineDirectory + "/Shaders/ShadowMapBindless.frag",
              defs)

          // Pipeline resource layouts
          .layoutBuilder
          // Global resource heaps
          .addDescriptorSet(heap.getDescriptorSetLayout())
          .addPushConstants<ShadowMapPushConstants>(VK_SHADER_STAGE_ALL);
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

    this->_shadowPass = RenderPass(
        app,
        extent,
        std::move(attachments),
        std::move(subpassBuilders));

    this->_shadowFrameBuffers.resize(lightCount);
    for (size_t i = 0; i < lightCount; ++i) {
      this->_shadowFrameBuffers[i] = FrameBuffer(
          app,
          this->_shadowPass,
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
  // Update light positions
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

  ShaderDefines defs{};
  defs.emplace("BINDLESS_SET", "0");

  subpassBuilder
      .pipelineBuilder
      // TODO: This is a hack to workaround incorrect winding of sphere
      // triangle indices - fix that instead of disabling backface culling
      .addVertexInputBinding<glm::vec3>(VK_VERTEX_INPUT_RATE_VERTEX)
      .addVertexAttribute(VertexAttributeType::VEC3, 0)

      // Vertex shader
      .addVertexShader(GEngineDirectory + "/Shaders/LightMesh.vert", defs)
      // Fragment shader
      .addFragmentShader(GEngineDirectory + "/Shaders/LightMesh.frag", defs)

      // Pipeline resource layouts
      .layoutBuilder
      // Global resources (view, projection, environment ma)
      .addDescriptorSet(globalDescriptorSetLayout);
}

void PointLightCollection::drawShadowMaps(
    Application& app,
    VkCommandBuffer commandBuffer,
    const FrameContext& frame,
    const std::vector<Model>& models,
    VkDescriptorSet globalSet) {

  // Shadow passes
  this->_shadowMap.transitionToAttachment(commandBuffer);

  for (uint32_t i = 0; i < this->_lights.size(); ++i) {
    const PointLight& light = this->_lights[i];

    ActiveRenderPass pass = this->_shadowPass.begin(
        app,
        commandBuffer,
        frame,
        this->_shadowFrameBuffers[i]);

    pass.setGlobalDescriptorSets(gsl::span(&globalSet, 1));

    // Draw models
    for (const Model& model : models) {
      // ShadowMapPushConstants constants{};
      // constants.primitiveIdx = 
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