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
} // namespace AltheaEngine

namespace std {
template <> struct hash<::AltheaEngine::KeyBinding> {
  size_t operator()(const ::AltheaEngine::KeyBinding& kb) const {
    hash<int> h;
    int combined = (kb.key << 5) ^ (kb.action << 3) ^ kb.mods;
    return h(combined);
  }
};
} // namespace std

namespace AltheaEngine {
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
  bool addBinding(const KeyBinding& binding, std::function<void()>&& callback);

  /**
   * @brief Remove the given binding if it exists in the current bindings.
   * 
   * @param binding The binding to remove.
   * @return Whether the binding was removed successfully. 
   */
  bool removeBinding(const KeyBinding& binding);

private:
  void _processKey(int key, int action, int mods);

  // TODO: Allow for multiple callbacks bound to same combinations?
  std::unordered_map<KeyBinding, std::function<void()>> _bindings{};
};
} // namespace AltheaEngine
