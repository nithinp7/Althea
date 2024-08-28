#pragma once

#include "BindlessHandle.h"
#include "Allocator.h"
#include "ComputePipeline.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace AltheaEngine {
class Application;
class GlobalHeap;

typedef uint32_t StencilQueryHandle;

class StencilQueryManager {
public:
  StencilQueryManager() = default;
  StencilQueryManager(const Application& app, VkDescriptorSetLayout heapLayout);

  void update(GlobalHeap& heap, VkCommandBuffer commandBuffer, TextureHandle stencil);

  StencilQueryHandle createStencilQuery(const glm::vec2& uv);

  bool getQueryResults(StencilQueryHandle handle, uint32_t& result);

private:
  static constexpr uint32_t NUM_QUERY_BUFFERS = MAX_FRAMES_IN_FLIGHT;
  static constexpr uint32_t QUERY_BATCH_SIZE = 32;
  static constexpr uint32_t NUM_RETAINED_QUERY_GENERATIONS = 32;

  struct QueryInputBatch {
    glm::vec2 inputs[QUERY_BATCH_SIZE];
  };
  std::vector<QueryInputBatch> m_inputBatches;

  struct QueryOutputBatch {
    uint32_t outputs[QUERY_BATCH_SIZE];
  };

  struct QueryGenerationResults {
    std::vector<QueryOutputBatch> batches;
    uint16_t generationIdx = 0;
  };
  std::array<QueryGenerationResults, NUM_RETAINED_QUERY_GENERATIONS> m_generationResults;

  uint16_t m_currentQueryIdx = 0;
  uint16_t m_currentGenerationIdx = 0;
  
  struct QueryBuffer {
    BufferHandle handle{};
    BufferAllocation allocation{};
    uint32_t batchCount = 0;
  };

  std::array<QueryBuffer, NUM_QUERY_BUFFERS> m_inputBuffers{};
  std::array<QueryBuffer, NUM_QUERY_BUFFERS> m_outputBuffers{};

  ComputePipeline m_queryCompute{};
};
} // namespace AltheaEngine