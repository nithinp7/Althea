#include "InputManager.h"
#include "Application.h"

namespace AltheaEngine {
InputManager::InputManager(GLFWwindow* window_) {
  glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode, int action, int mode) {
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    app->getInputManager()._processKey(key, action, mode);
  });
}

void InputManager::_processKey(int key, int action, int mods) {
  auto bindingIt = this->_bindings.find({key, action, mods});
  if (bindingIt != this->_bindings.end()) {
    // Invoke callback for this binding
    (bindingIt->second)();
  }
}
} // namespace AltheaEngine