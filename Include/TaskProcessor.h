#pragma once

#include <CesiumAsync/ITaskProcessor.h>

class TaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  void startTask(std::function<void()> f) override;
};
