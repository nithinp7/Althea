#include "DeletionTasks.h"

#include <vector>

namespace AltheaEngine {
void DeletionTasks::tick(const FrameContext& frame) {
  auto it = std::remove_if(
      this->_deletionTasks.begin(),
      this->_deletionTasks.end(),
      [index = frame.frameRingBufferIndex](const DeletionTask& task) {
        if (task.frameRingBufferIndex == index) {
          task.deleter();
          return true;
        }

        return false;
      });
  this->_deletionTasks.erase(it, this->_deletionTasks.end());
}

void DeletionTasks::addDeletionTask(DeletionTask&& deletionTask) {
  this->_deletionTasks.emplace_back(std::move(deletionTask));
}

void DeletionTasks::flush() {
  for (DeletionTask& task : this->_deletionTasks) {
    task.deleter();
  }

  this->_deletionTasks.clear();
}
} // namespace AltheaEngine