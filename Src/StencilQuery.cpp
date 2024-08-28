#include "StencilQuery.h"

#include "Application.h"
#include "BufferUtilities.h"
#include "GlobalHeap.h"

namespace AltheaEngine {
namespace {
struct PushConstants {
  uint32_t stencilHandle;
  uint32_t inputBufferHandle;
  uint32_t outputBufferHandle;
  uint32_t queryCount;
};
} // namespace
StencilQueryManager::StencilQueryManager(
    const Application& app,
    VkDescriptorSetLayout heapLayout) {
  ComputePipelineBuilder builder{};
  builder.setComputeShader(
      GEngineDirectory + "/Shaders/Query/StencilQuery.comp");
  builder.layoutBuilder.addDescriptorSet(heapLayout);
  builder.layoutBuilder.addPushConstants<PushConstants>(
      VK_SHADER_STAGE_COMPUTE_BIT);

  m_queryCompute = ComputePipeline(app, std::move(builder));
}

void StencilQueryManager::update(
    GlobalHeap& heap,
    VkCommandBuffer commandBuffer,
    TextureHandle stencil) {

  uint16_t bufIdx = m_currentGenerationIdx % NUM_QUERY_BUFFERS;
  QueryBuffer& inputBuffer = m_inputBuffers[bufIdx];
  QueryBuffer& outputBuffer = m_outputBuffers[bufIdx];

  // process previous queries
  if (outputBuffer.batchCount > 0) {
    // intentional underflow
    uint16_t finishedGeneration = m_currentGenerationIdx - NUM_QUERY_BUFFERS;
    QueryGenerationResults& results = m_generationResults
        [finishedGeneration % NUM_RETAINED_QUERY_GENERATIONS];
    results.generationIdx = finishedGeneration;
    results.batches.resize(outputBuffer.batchCount);

    void* p = outputBuffer.allocation.mapMemory();
    memcpy(
        results.batches.data(),
        p,
        sizeof(QueryOutputBatch) * outputBuffer.batchCount);
    outputBuffer.allocation.unmapMemory();

    outputBuffer.batchCount = 0;
  }

  uint32_t queryCount = m_currentQueryIdx;

  ++m_currentGenerationIdx;
  m_currentQueryIdx = 0;

  // issue new queries

  if (queryCount == 0)
    return;

  uint32_t batchCount = (queryCount + QUERY_BATCH_SIZE - 1) / QUERY_BATCH_SIZE;

  if (!inputBuffer.handle.isValid()) {
    inputBuffer.handle = heap.registerBuffer();
  }

  if (inputBuffer.batchCount < batchCount) {
    VmaAllocationCreateInfo info{};
    info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    info.usage = VMA_MEMORY_USAGE_AUTO;

    size_t bufferSize = sizeof(QueryInputBatch) * batchCount;
    inputBuffer.allocation = BufferUtilities::createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        info);

    inputBuffer.batchCount = batchCount;
    heap.updateStorageBuffer(
        inputBuffer.handle,
        inputBuffer.allocation.getBuffer(),
        0,
        bufferSize);
  }

  {
    void* p = inputBuffer.allocation.mapMemory();
    memcpy(p, m_inputBatches.data(), sizeof(QueryInputBatch) * batchCount);
    inputBuffer.allocation.unmapMemory();
  }

  if (!outputBuffer.handle.isValid()) {
    outputBuffer.handle = heap.registerBuffer();
  }

  if (outputBuffer.batchCount < batchCount) {
    VmaAllocationCreateInfo info{};
    info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    info.usage = VMA_MEMORY_USAGE_AUTO;

    size_t bufferSize = sizeof(QueryOutputBatch) * batchCount;
    outputBuffer.allocation = BufferUtilities::createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        info);

    outputBuffer.batchCount = batchCount;
    heap.updateStorageBuffer(
        outputBuffer.handle,
        outputBuffer.allocation.getBuffer(),
        0,
        bufferSize);
  }

  PushConstants push{};
  push.stencilHandle = stencil.index;
  push.inputBufferHandle = inputBuffer.handle.index;
  push.outputBufferHandle = outputBuffer.handle.index;
  push.queryCount = queryCount;

  m_queryCompute.bindPipeline(commandBuffer);
  m_queryCompute.bindDescriptorSet(commandBuffer, heap.getDescriptorSet());
  m_queryCompute.setPushConstants(commandBuffer, push);

  vkCmdDispatch(commandBuffer, batchCount, 1, 1);
}

StencilQueryHandle
StencilQueryManager::createStencilQuery(const glm::vec2& uv) {
  uint32_t queryIdx = m_currentQueryIdx++;
  uint32_t batchCount = (m_currentQueryIdx + QUERY_BATCH_SIZE - 1) / QUERY_BATCH_SIZE;
  if (m_inputBatches.size() < batchCount) {
    m_inputBatches.resize(batchCount);
  }

  m_inputBatches[queryIdx / QUERY_BATCH_SIZE]
      .inputs[queryIdx % QUERY_BATCH_SIZE] = uv;

  StencilQueryHandle handle =
      ((uint32_t)m_currentGenerationIdx << 16) | queryIdx;
  return handle;
}

bool StencilQueryManager::getQueryResults(
    StencilQueryHandle handle,
    uint32_t& result) {
  uint32_t generationIdx = handle >> 16;
  uint32_t queryIdx = handle & 0xffff;
  for (const QueryGenerationResults& generation : m_generationResults) {
    if (generation.batches.size() && generationIdx == generation.generationIdx) {
      result = generation.batches[queryIdx / QUERY_BATCH_SIZE]
                   .outputs[queryIdx % QUERY_BATCH_SIZE];
      return true;
    }
  }

  return false;
}
} // namespace AltheaEngine