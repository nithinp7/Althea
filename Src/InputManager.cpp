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

bool InputManager::addBinding(const KeyBinding& binding, std::function<void()>&& callback) {
  return this->_bindings.insert(std::make_pair(binding, std::move(callback))).second;  
}

bool InputManager::removeBinding(const KeyBinding& binding) {
  return !!this->_bindings.erase(binding);
}
} // namespace AltheaEngine