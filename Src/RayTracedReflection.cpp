#include "RayTracedReflection.h"

#include "Application.h"

#include <cstdint>

namespace AltheaEngine {

RayTracedReflection::RayTracedReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    // TODO: standardize global descriptor set
    VkDescriptorSetLayout globalSetLayout,
    VkAccelerationStructureKHR tlas,
    const GBufferResources& gBuffer,
    const ShaderDefines& shaderDefs)
    : _reflectionBuffer(app, commandBuffer) {

  // Setup material needed for reflection pass
  {
    DescriptorSetLayoutBuilder layoutBuilder{};
    GBufferResources::buildMaterial(layoutBuilder);
    layoutBuilder.addAccelerationStructureBinding(
        VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    layoutBuilder.addStorageImageBinding(VK_SHADER_STAGE_RAYGEN_BIT_KHR);

    this->_pReflectionMaterialAllocator =
        std::make_unique<DescriptorSetAllocator>(app, layoutBuilder, 1);
    this->_pReflectionMaterial =
        std::make_unique<Material>(app, *this->_pReflectionMaterialAllocator);
    ResourcesAssignment assignment = this->_pReflectionMaterial->assign();
    gBuffer.bindTextures(assignment);
    assignment.bindAccelerationStructure(tlas);
    assignment.bindStorageImage(
        this->_reflectionBuffer.getReflectionBufferTargetView(),
        this->_reflectionBuffer.getReflectionBufferTargetSampler());
  }

  // Setup reflection pass
  {
    RayTracingPipelineBuilder pipelineBuilder;
    pipelineBuilder.setRayGenShader(
        GEngineDirectory + "/Shaders/RayTracing/Reflections.glsl",
        shaderDefs);
    pipelineBuilder.addClosestHitShader(
        GEngineDirectory + "/Shaders/RayTracing/ClosestHit.glsl",
        shaderDefs);
    pipelineBuilder.addMissShader(
        GEngineDirectory + "/Shaders/RayTracing/Miss.glsl",
        shaderDefs);

    pipelineBuilder.layoutBuilder.addDescriptorSet(globalSetLayout);
    pipelineBuilder.layoutBuilder.addDescriptorSet(
        this->_pReflectionMaterialAllocator->getLayout());

    this->_pReflectionPass =
        std::make_unique<RayTracingPipeline>(app, std::move(pipelineBuilder));
  }

  this->_sbt = ShaderBindingTable(app, *this->_pReflectionPass);
}

void RayTracedReflection::transitionToAttachment(
    VkCommandBuffer commandBuffer) {
  this->_reflectionBuffer.transitionToAttachment(commandBuffer);
}

void RayTracedReflection::captureReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkDescriptorSet globalSet,
    const FrameContext& context) {
  // TODO make transition private function?
  this->transitionToAttachment(commandBuffer);

  vkCmdBindPipeline(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
      *this->_pReflectionPass);
  // VkDescriptorSet rtDescSets = { globalDescriptorSet, this?}
  {
    VkDescriptorSet sets[2] = {
        globalSet,
        this->_pReflectionMaterial->getCurrentDescriptorSet(context)};
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        this->_pReflectionPass->getLayout(),
        0,
        2,
        sets,
        0,
        nullptr);

    const VkExtent2D& extent = app.getSwapChainExtent();
    Application::vkCmdTraceRaysKHR(
        commandBuffer,
        &this->_sbt.getRayGenRegion(),
        &this->_sbt.getMissRegion(),
        &this->_sbt.getHitRegion(),
        &this->_sbt.getCallRegion(),
        extent.width, // 1080,
        extent.height, // 960,
        1);
  }
}

void RayTracedReflection::convolveReflectionBuffer(
    const Application& app,
    VkCommandBuffer commandBuffer,
    const FrameContext& context) {
  this->_reflectionBuffer.convolveReflectionBuffer(app, commandBuffer, context);
}

void RayTracedReflection::bindTexture(ResourcesAssignment& assignment) const {
  this->_reflectionBuffer.bindTexture(assignment);
}

} // namespace AltheaEngine