
#include "RayTracingResources.h"

#include "Application.h"
#include "GlobalHeap.h"
#include "Model.h"
#include "SingleTimeCommandBuffer.h"

namespace AltheaEngine {
RayTracingResources::RayTracingResources(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap,
    const RayTracingResourcesBuilder& builder) {
  m_accelerationStructure =
      AccelerationStructure(app, commandBuffer, *builder.models);
  m_accelerationStructure.registerToHeap(heap);

  ImageOptions imageOptions{};
  imageOptions.width = app.getSwapChainExtent().width;
  imageOptions.height = app.getSwapChainExtent().height;
  imageOptions.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  m_target.image = Image(app, imageOptions);
  m_target.view = ImageView(app, m_target.image, {});
  m_target.sampler = Sampler(app, {});

  m_target.registerToImageHeap(heap);
  m_target.registerToTextureHeap(heap);
}

void RayTracingResources::transitionToTarget(VkCommandBuffer commandBuffer) {
  m_target.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_ACCESS_SHADER_WRITE_BIT,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
}

void RayTracingResources::transitionToTexture(VkCommandBuffer commandBuffer) {
  m_target.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

RayTracingHandles RayTracingResources::getHandles() const {
  RayTracingHandles handles{};
  handles.tlasHandle = m_accelerationStructure.getTlasHandle().index;
  handles.targetImgHandle = m_target.imageHandle.index;
  handles.targetTexHandle = m_target.textureHandle.index;

  return handles;
}
} // namespace AltheaEngine