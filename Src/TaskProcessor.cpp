
#include "TaskProcessor.h"

#include <thread>

namespace AltheaEngine {
void TaskProcessor::startTask(std::function<void()> f) {
  std::thread(f).detach();
}
} // namespace AltheaEngine
