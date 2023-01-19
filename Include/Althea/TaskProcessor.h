#pragma once

#include "Library.h"

#include <CesiumAsync/ITaskProcessor.h>

namespace AltheaEngine {
class ALTHEA_API TaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  void startTask(std::function<void()> f) override;
};
} // namespace AltheaEngine
