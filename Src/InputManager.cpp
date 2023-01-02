#include "InputManager.h"

#include "Application.h"


namespace AltheaEngine {
namespace {
struct NdcCoord {
  double x = 0.0;
  double y = 0.0;
};

NdcCoord
screenToNdc(const VkExtent2D& screenDims, double screenX, double screenY) {
  // Convert to normalized device coordinates (NDC) with (-1, -1) in
  // the bottom left.
  return {
      2.0 * screenX / (double)screenDims.width - 1.0,
      1.0 - 2.0 * screenY / (double)screenDims.height};
}
} // namespace

InputManager::InputManager(GLFWwindow* window_) {
  glfwSetKeyCallback(
      window_,
      [](GLFWwindow* window, int key, int scancode, int action, int mode) {
        Application* app = (Application*)glfwGetWindowUserPointer(window);
        app->getInputManager()._processKey(key, action, mode);
      });

  glfwSetMouseButtonCallback(
      window_,
      [](GLFWwindow* window, int button, int action, int mode) {
        Application* app = (Application*)glfwGetWindowUserPointer(window);
        app->getInputManager()._processMouseButton(button, action, mode);
      });

  glfwSetCursorPosCallback(
      window_,
      [](GLFWwindow* window, double xPos, double yPos) {
        Application* app = (Application*)glfwGetWindowUserPointer(window);

        const VkExtent2D& screenDims = app->getSwapChainExtent();
        NdcCoord ndc = screenToNdc(screenDims, xPos, yPos);
        app->getInputManager()._updateMousePos(ndc.x, ndc.y);
      });

  glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  this->addKeyBinding(
      {GLFW_KEY_F1, GLFW_PRESS, 0},
      std::bind(
          [](GLFWwindow* window, InputManager* inputManager) {
            if (inputManager->_cursorHidden) {
              glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
              inputManager->_cursorHidden = false;
            } else {
              glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
              inputManager->_cursorHidden = true;
            }

            // Update the mouse position to notify clients of cursor change.
            double xPos;
            double yPos;
            glfwGetCursorPos(window, &xPos, &yPos);

            Application* app = (Application*)glfwGetWindowUserPointer(window);

            const VkExtent2D& screenDims = app->getSwapChainExtent();
            NdcCoord ndc = screenToNdc(screenDims, xPos, yPos);
            app->getInputManager()._updateMousePos(ndc.x, ndc.y);
            inputManager->_updateMousePos(xPos, yPos);
          },
          window_,
          this));
}

void InputManager::_processKey(int key, int action, int mods) {
  auto bindingIt = this->_keyBindings.find({key, action, mods});
  if (bindingIt != this->_keyBindings.end()) {
    // Invoke callback for this binding
    bindingIt->second();
  }
}

void InputManager::_processMouseButton(int button, int action, int mods) {
  auto bindingIt = this->_mouseButtonBindings.find({button, action, mods});
  if (bindingIt != this->_mouseButtonBindings.end()) {
    // Invoke callback for this binding
    bindingIt->second();
  }
}

void InputManager::_updateMousePos(double xPos, double yPos) {
  for (auto& mouseCallback : this->_mousePositionCallbacks) {
    mouseCallback.second(xPos, yPos, this->_cursorHidden);
  }
}

bool InputManager::addKeyBinding(
    const KeyBinding& binding,
    std::function<void()>&& callback) {
  return this->_keyBindings.emplace(binding, std::move(callback)).second;
}

bool InputManager::removeKeyBinding(const KeyBinding& binding) {
  return !!this->_keyBindings.erase(binding);
}

bool InputManager::addMouseBinding(
    const MouseButtonBinding& binding,
    MouseButtonBindingCallback&& callback) {
  return this->_mouseButtonBindings.emplace(binding, std::move(callback))
      .second;
}

bool InputManager::removeMouseBinding(const MouseButtonBinding& binding) {
  return !!this->_mouseButtonBindings.erase(binding);
}

uint32_t
InputManager::addMousePositionCallback(MousePositionCallback&& callback) {
  this->_mousePositionCallbacks.emplace(
      this->_currentMouseCallbackId,
      std::move(callback));
  return this->_currentMouseCallbackId++;
}

bool InputManager::removeMousePositionCallback(uint32_t callbackId) {
  return !!this->_mousePositionCallbacks.erase(callbackId);
}
} // namespace AltheaEngine