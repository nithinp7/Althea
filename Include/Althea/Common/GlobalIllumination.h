#pragma once
// Structs and defines corresponding to Shaders/GIResources.glsl - should be
// kept in sync

#include <glm/glm.hpp>

#include <cstdint>

namespace AltheaEngine {
namespace GlobalIllumination {

struct GISample {
  glm::vec4 position;
  
  alignas(16) glm::vec3 wiw;
  alignas(8) float W;
  alignas(16) glm::vec3 Li;
  alignas(4) float padding;
};

struct Reservoir {
  GISample s;
};

struct Probe {
  GISample samples[8];
  alignas(16) glm::vec3 position;
  alignas(4) float padding; // ??
};

enum LiveEditFlags {
  LEF_NONE = 0,
  LEF_LIGHT_SAMPLING_MODE = 1,
  LEF_DISABLE_ENV_MAP = 1<<1
};

struct LiveEditValues {
  float temporalBlend = 0.25f;
  float depthDiscrepancyTolerance = 0.5;
  float spatialResamplingRadius = 0.5;
  float lightIntensity = 0.5;
  uint32_t flags = 0;
};

struct Uniforms {
  uint32_t tlas;

  uint32_t colorSampler;
  uint32_t colorTarget;

  uint32_t targetWidth;
  uint32_t targetHeight;

  uint32_t writeIndex;

  uint32_t reservoirHeap;
  uint32_t reservoirsPerBuffer;

  uint32_t frameNumber;

  uint32_t probesInfo; // VkDrawIndexedIndirectCommand
  uint32_t probes;
  uint32_t spatialHash;

  LiveEditValues liveValues;
};
} // namespace GlobalIllumination
} // namespace AltheaEngine