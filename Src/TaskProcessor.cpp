
#include "TaskProcessor.h"

#include <thread>

void TaskProcessor::startTask(std::function<void()> f) {
  std::thread thread_obj(f);
}
