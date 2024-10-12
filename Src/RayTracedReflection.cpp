#include "RayTracedReflection.h"

#include "Application.h"

#include <cstdint>
#include <iostream>

namespace AltheaEngine {

RayTracedReflection::RayTracedReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    GlobalHeap& heap,
    VkAccelerationStructureKHR tlas,
    const GBufferResources& gBuffer,
    const ShaderDefines& shaderDefs)
    : _reflectionBuffer(app, commandBuffer, heap) {

  // TODO: Switch this to bindless, this looks broken after removal of old
  // descriptor set code.
  
  // Setup material needed for reflection pass
  {
    // DescriptorSetLayoutBuilder layoutBuilder{};
    // GBufferResources::buildMaterial(layoutBuilder);
    // layoutBuilder.addAccelerationStructureBinding(
    //     VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    // layoutBuilder.addStorageImageBinding(VK_SHADER_STAGE_RAYGEN_BIT_KHR);

    // this->_pReflectionMaterialAllocator =
    //     std::make_unique<DescriptorSetAllocator>(app, layoutBuilder, 1);
    // // this->_pReflectionMaterial =
    // //     std::make_unique<Material>(app, *this->_pReflectionMaterialAllocator);
    // ResourcesAssignment assignment;// = this->_pReflectionMaterial->assign();
    // gBuffer.bindTextures(assignment);
    // assignment.bindAccelerationStructure(tlas);
    // assignment.bindStorageImage(
    //     this->_reflectionBuffer.getReflectionBufferTargetView(),
    //     this->_reflectionBuffer.getReflectionBufferTargetSampler());
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

    pipelineBuilder.layoutBuilder.addDescriptorSet(heap.getDescriptorSetLayout());
    pipelineBuilder.layoutBuilder.addDescriptorSet(
        this->_pReflectionMaterialAllocator->getLayout());

    this->_pReflectionPass =
        std::make_unique<RayTracingPipeline>(app, std::move(pipelineBuilder));
  }
}

void RayTracedReflection::captureReflection(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkDescriptorSet globalSet,
    const FrameContext& context) {
  this->_reflectionBuffer.transitionToStorageImageWrite(
      commandBuffer,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

  vkCmdBindPipeline(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
      *this->_pReflectionPass);
  // VkDescriptorSet rtDescSets = { globalDescriptorSet, this?}
  {
    VkDescriptorSet sets[2] = {
        globalSet,
        0};//this->_pReflectionMaterial->getCurrentDescriptorSet(context)};
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
    this->_pReflectionPass->traceRays(extent, commandBuffer);
  }
}

void RayTracedReflection::convolveReflectionBuffer(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkDescriptorSet heapSet,
    const FrameContext& context) {
  this->_reflectionBuffer.convolveReflectionBuffer(
      app,
      commandBuffer,
      heapSet,
      context,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_ACCESS_SHADER_WRITE_BIT,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
}

void RayTracedReflection::bindTexture(ResourcesAssignment& assignment) const {
  this->_reflectionBuffer.bindTexture(assignment);
}

void RayTracedReflection::tryRecompileShaders(Application& app) {
  if (this->_pReflectionPass->recompileStaleShaders()) {
    if (this->_pReflectionPass->hasShaderRecompileErrors()) {
      std::cout << this->_pReflectionPass->getShaderRecompileErrors() << "\n";
    } else {
      this->_pReflectionPass->recreatePipeline(app);
    }
  }
}
} // namespace AltheaEngine