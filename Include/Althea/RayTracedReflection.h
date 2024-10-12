#pragma once

#include "DeferredRendering.h"
#include "DescriptorSet.h"
#include "FrameBuffer.h"
#include "FrameContext.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "ImageResource.h"
#include "ImageView.h"
#include "Library.h"
#include "PerFrameResources.h"
#include "RayTracingPipeline.h"
#include "ReflectionBuffer.h"
#include "ResourcesAssignment.h"
#include "Sampler.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;

class ALTHEA_API RayTracedReflection {
public:
  RayTracedReflection() = default;
  RayTracedReflection(
      const Application& app,
      VkCommandBuffer commandBuffer,
      GlobalHeap& heap,
      VkAccelerationStructureKHR tlas,
      const GBufferResources& gBuffer,
      const ShaderDefines& shaderDefs);
  void captureReflection(
      const Application& app,
      VkCommandBuffer commandBuffer,
      VkDescriptorSet globalSet,
      const FrameContext& context);
  void convolveReflectionBuffer(
      const Application& app,
      VkCommandBuffer commandBuffer,
      VkDescriptorSet heapSet,
      const FrameContext& context);

  void bindTexture(ResourcesAssignment& assignment) const;

  void tryRecompileShaders(Application& app);

private:
  ReflectionBuffer _reflectionBuffer;

  std::unique_ptr<RayTracingPipeline> _pReflectionPass;
  std::unique_ptr<DescriptorSetAllocator> _pReflectionMaterialAllocator;
  // std::unique_ptr<Material> _pReflectionMaterial;
};
} // namespace AltheaEngine