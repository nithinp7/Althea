#pragma once

#include <CesiumAsync/ITaskProcessor.h>

namespace AltheaEngine {
class TaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  void startTask(std::function<void()> f) override;
};
} // namespace AltheaEngine

