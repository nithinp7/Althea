#include "InputManager.h"

#include "Application.h"
#include "InputMask.h"

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

InputManager::InputManager(GLFWwindow* window_) : _pWindow(window_) {
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
            inputManager->setMouseCursorHidden(!inputManager->_cursorHidden);
          },
          window_,
          this));
}

void InputManager::_processKey(int key, int action, int mods) {
  uint32_t bit = 0;

  switch (key) {
  case GLFW_KEY_A:
    bit = INPUT_BIT_A;
    break;
  case GLFW_KEY_B:
    bit = INPUT_BIT_B;
    break;
  case GLFW_KEY_C:
    bit = INPUT_BIT_C;
    break;
  case GLFW_KEY_D:
    bit = INPUT_BIT_D;
    break;
  case GLFW_KEY_E:
    bit = INPUT_BIT_E;
    break;
  case GLFW_KEY_F:
    bit = INPUT_BIT_F;
    break;
  case GLFW_KEY_G:
    bit = INPUT_BIT_G;
    break;
  case GLFW_KEY_H:
    bit = INPUT_BIT_H;
    break;
  case GLFW_KEY_I:
    bit = INPUT_BIT_I;
    break;
  case GLFW_KEY_J:
    bit = INPUT_BIT_J;
    break;
  case GLFW_KEY_K:
    bit = INPUT_BIT_K;
    break;
  case GLFW_KEY_L:
    bit = INPUT_BIT_L;
    break;
  case GLFW_KEY_M:
    bit = INPUT_BIT_M;
    break;
  case GLFW_KEY_N:
    bit = INPUT_BIT_N;
    break;
  case GLFW_KEY_O:
    bit = INPUT_BIT_O;
    break;
  case GLFW_KEY_P:
    bit = INPUT_BIT_P;
    break;
  case GLFW_KEY_Q:
    bit = INPUT_BIT_Q;
    break;
  case GLFW_KEY_R:
    bit = INPUT_BIT_R;
    break;
  case GLFW_KEY_S:
    bit = INPUT_BIT_S;
    break;
  case GLFW_KEY_T:
    bit = INPUT_BIT_T;
    break;
  case GLFW_KEY_U:
    bit = INPUT_BIT_U;
    break;
  case GLFW_KEY_V:
    bit = INPUT_BIT_V;
    break;
  case GLFW_KEY_W:
    bit = INPUT_BIT_W;
    break;
  case GLFW_KEY_X:
    bit = INPUT_BIT_X;
    break;
  case GLFW_KEY_Y:
    bit = INPUT_BIT_Y;
    break;
  case GLFW_KEY_Z:
    bit = INPUT_BIT_Z;
    break;
  case GLFW_KEY_SPACE:
    bit = INPUT_BIT_SPACE;
    break;
  case GLFW_KEY_LEFT_CONTROL:
    bit = INPUT_BIT_CTRL;
    break;
  case GLFW_KEY_LEFT_ALT:
    bit = INPUT_BIT_ALT;
    break;
  case GLFW_KEY_LEFT_SHIFT:
    bit = INPUT_BIT_SHIFT;
    break;
  }

  if (action == GLFW_RELEASE) {
    this->_currentInputMask &= ~bit;
  } else {
    this->_currentInputMask |= bit;
  }

  auto bindingIt = this->_keyBindings.find({key, action, mods});
  if (bindingIt != this->_keyBindings.end()) {
    // Invoke callback for this binding
    bindingIt->second();
  }
}

void InputManager::_processMouseButton(int button, int action, int mods) {
  uint32_t bit = 0;
  switch (button) {
  case GLFW_MOUSE_BUTTON_LEFT:
    bit = INPUT_BIT_LEFT_MOUSE;
    break;
  case GLFW_MOUSE_BUTTON_RIGHT:
    bit = INPUT_BIT_RIGHT_MOUSE;
    break;
  }

  if (action == GLFW_RELEASE) {
    this->_currentInputMask &= ~bit;
  } else {
    this->_currentInputMask |= bit;
  }

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

void InputManager::setMouseCursorHidden(bool cursorHidden) {
  if (this->_cursorHidden == cursorHidden) {
    return;
  }

  Application* app = (Application*)glfwGetWindowUserPointer(this->_pWindow);
  const VkExtent2D& screenDims = app->getSwapChainExtent();

  if (this->_cursorHidden) {
    glfwSetInputMode(this->_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    this->_cursorHidden = false;
  } else {
    glfwSetInputMode(this->_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    this->_cursorHidden = true;
  }

  glfwSetCursorPos(
      this->_pWindow,
      screenDims.width / 2.0,
      screenDims.height / 2.0);

  // Update the mouse position to notify clients of cursor change.
  double xPos;
  double yPos;
  glfwGetCursorPos(_pWindow, &xPos, &yPos);

  NdcCoord ndc = screenToNdc(screenDims, xPos, yPos);
  this->_updateMousePos(ndc.x, ndc.y);
}

InputManager::MousePos InputManager::getCurrentMousePos() const {
  // Update the mouse position to notify clients of cursor change.
  MousePos mPos;
  glfwGetCursorPos(_pWindow, &mPos.x, &mPos.y);
  int width, height;
  glfwGetWindowSize(_pWindow, &width, &height);
  NdcCoord coord = screenToNdc({ (uint32_t)width, (uint32_t)height }, mPos.x, mPos.y);
  return { coord.x, coord.y };
}
} // namespace AltheaEngine