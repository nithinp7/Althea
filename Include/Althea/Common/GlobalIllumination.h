#pragma once
// Structs and defines corresponding to Shaders/GIResources.glsl - should be
// kept in sync

#include <glm/glm.hpp>

#include <cstdint>

namespace AltheaEngine {
namespace GlobalIllumination {

struct GISample {
  alignas(16) glm::vec3 wiw;
  alignas(8) float W;
  alignas(16) glm::vec3 Li;
  alignas(4) float padding;
};

struct Reservoir {
  GISample samples[8];
  uint32_t sampleCount = 0;
  float wSum = 0.0f;
};

struct Probe {
  GISample samples[8];
  alignas(16) glm::vec3 position;
  alignas(4) float padding; // ??
};

struct LiveEditValues {
  float temporalBlend = 0.25f;
  float slider1;
  float slider2;
  bool checkbox1;
  bool checkbox2;
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