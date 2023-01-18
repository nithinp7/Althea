#pragma once

#include "Library.h"

#include "FrameContext.h"

#include <cstdint>
#include <functional>
#include <vector>

struct ALTHEA_API DeletionTask {
  std::function<void()> deleter;
  uint32_t frameRingBufferIndex;

  DeletionTask(DeletionTask&& rhs) = default;
  DeletionTask& operator=(DeletionTask&& rhs) = default;

  DeletionTask(const DeletionTask& rhs) = delete;
  DeletionTask& operator=(const DeletionTask& rhs) = delete;
};

namespace AltheaEngine {
class ALTHEA_API DeletionTasks {
public:
  void tick(const FrameContext& frame);
  void addDeletionTask(DeletionTask&& task);
  void flush();

private:
  std::vector<DeletionTask> _deletionTasks;
};
} // namespace AltheaEngine