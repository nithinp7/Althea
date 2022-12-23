#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <unordered_map>
#include <functional>

namespace AltheaEngine {
struct KeyBinding {
  int key;
  int action;
  int mods;
  
  constexpr bool operator==(const KeyBinding& other) const noexcept {
    return key == other.key && action == other.action && mods == other.mods;
  }
  
  constexpr bool operator!=(const KeyBinding& other) const noexcept {
    return key != other.key || action != other.action || mods != other.mods;
  }
};

struct MouseButtonBinding {
  int button;
  int action;
  int mods;
  
  constexpr bool operator==(const MouseButtonBinding& other) const noexcept {
    return button == other.button && action == other.action && mods == other.mods;
  }
  
  constexpr bool operator!=(const MouseButtonBinding& other) const noexcept {
    return button != other.button || action != other.action || mods != other.mods;
  }
};
} // namespace AltheaEngine

namespace std {
template <> struct hash<::AltheaEngine::KeyBinding> {
  size_t operator()(const ::AltheaEngine::KeyBinding& kb) const {
    hash<int> h;
    int combined = (kb.key << 5) ^ (kb.action << 3) ^ kb.mods;
    return h(combined);
  }
};

template <> struct hash<::AltheaEngine::MouseButtonBinding> {
  size_t operator()(const ::AltheaEngine::MouseButtonBinding& mb) const {
    hash<int> h;
    int combined = (mb.button << 5) ^ (mb.action << 3) ^ mb.mods;
    return h(combined);
  }
};
} // namespace std

namespace AltheaEngine {

typedef std::function<void()> KeyBindingCallback;
typedef std::function<void()> MouseButtonBindingCallback;
typedef std::function<void(double, double, bool)> MousePositionCallback;

class InputManager {
public:
  InputManager(GLFWwindow* window);
  
  /**
   * @brief Bind a callback to a specific keystroke-action-modifiers if there
   * is not already a binding for the combination.
   * 
   * @param binding The key combination to bind.
   * @param callback The callback to invoke when the key combination happens.
   * @return Whether the binding was added successfully.
   */
  bool addKeyBinding(const KeyBinding& binding, KeyBindingCallback&& callback);

  /**
   * @brief Remove the given binding if it exists in the current bindings.
   * 
   * @param binding The binding to remove.
   * @return Whether the binding was removed successfully. 
   */
  bool removeKeyBinding(const KeyBinding& binding);

  /**
   * @brief Bind a callback to a specific mouse button action + mods if there
   * is not already a binding for the combination.
   * 
   * @param binding The mouse combination to bind.
   * @param callback The callback to invoke when the mouse combination happens.
   * @return Whether the binding was added successfully.
   */
  bool addMouseBinding(const MouseButtonBinding& binding, MouseButtonBindingCallback&& callback);

  /**
   * @brief Remove the given binding if it exists in the current bindings.
   * 
   * @param binding The binding to remove.
   * @return Whether the binding was removed successfully. 
   */
  bool removeMouseBinding(const MouseButtonBinding& binding);

  /**
   * @brief Add a callback for mouse position updates.
   * 
   * @param callback The mouse position callback to add.
   * @return The registered index of this callback. Use this index to later
   * remove the callback.
   */
  uint32_t addMousePositionCallback(MousePositionCallback&& callback);

  /**
   * @brief Remove a mouse position callback.
   * 
   * @param callbackId The ID of this callback, returned from addMouseCallback(...).
   * @return Whether the callback was successfully removed. 
   */
  bool removeMousePositionCallback(uint32_t callbackId);

private:
  void _processKey(int key, int action, int mods);
  void _processMouseButton(int button, int action, int mods);
  void _updateMousePos(double xPos, double yPos);

  // TODO: Allow for multiple callbacks bound to same combinations?
  std::unordered_map<KeyBinding, KeyBindingCallback> _keyBindings{};
  std::unordered_map<MouseButtonBinding, MouseButtonBindingCallback> _mouseButtonBindings{};
  std::unordered_map<uint32_t, MousePositionCallback> _mousePositionCallbacks{};

  bool _cursorHidden = true;

  uint32_t _currentMouseCallbackId = 0;
};
} // namespace AltheaEngine
