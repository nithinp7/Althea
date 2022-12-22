#include "CameraController.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>

namespace AltheaEngine {
CameraController::CameraController(
    InputManager& inputManager, 
    float fovDegrees, 
    float aspectRatio)
    : _camera(fovDegrees, aspectRatio, 0.1f, 1000.0f) {
  // TODO: allow bindings from config file

  // Bind inputs. 
  inputManager.addBinding({GLFW_KEY_W, GLFW_REPEAT, 0}, std::bind(&CameraController::_moveForward, this));
  inputManager.addBinding({GLFW_KEY_W, GLFW_PRESS, 0}, std::bind(&CameraController::_moveForward, this));
  inputManager.addBinding({GLFW_KEY_A, GLFW_REPEAT, 0}, std::bind(&CameraController::_strafeLeft, this));
  inputManager.addBinding({GLFW_KEY_A, GLFW_PRESS, 0}, std::bind(&CameraController::_strafeLeft, this));
  inputManager.addBinding({GLFW_KEY_S, GLFW_REPEAT, 0}, std::bind(&CameraController::_moveBackward, this));
  inputManager.addBinding({GLFW_KEY_S, GLFW_PRESS, 0}, std::bind(&CameraController::_moveBackward, this));
  inputManager.addBinding({GLFW_KEY_D, GLFW_REPEAT, 0}, std::bind(&CameraController::_strafeRight, this));
  inputManager.addBinding({GLFW_KEY_D, GLFW_PRESS, 0}, std::bind(&CameraController::_strafeRight, this));
  inputManager.addBinding({GLFW_KEY_Q, GLFW_REPEAT, 0}, std::bind(&CameraController::_descend, this));
  inputManager.addBinding({GLFW_KEY_Q, GLFW_PRESS, 0}, std::bind(&CameraController::_descend, this));
  inputManager.addBinding({GLFW_KEY_E, GLFW_REPEAT, 0}, std::bind(&CameraController::_ascend, this));
  inputManager.addBinding({GLFW_KEY_E, GLFW_PRESS, 0}, std::bind(&CameraController::_ascend, this));
}

CameraController::~CameraController() {
  // TODO: unbind?
}

void CameraController::tick(float deltaTime) {

}

static void moveLocal(Camera& camera, const glm::vec3& localDisplacement) {
  const glm::mat4& transform = camera.getTransform();

  glm::vec3 newPos(transform[3] + transform * glm::vec4(localDisplacement, 0.0f));
  camera.setPosition(newPos);
}

void CameraController::_moveForward() {
  moveLocal(this->_camera, glm::vec3(0.0f, 0.0f, -this->_movementSpeed));  
}

void CameraController::_moveBackward() {
  moveLocal(this->_camera, glm::vec3(0.0f, 0.0f, this->_movementSpeed));    
}

void CameraController::_strafeLeft() {
  moveLocal(this->_camera, glm::vec3(-this->_movementSpeed, 0.0f, 0.0f));    
}

void CameraController::_strafeRight() {
  moveLocal(this->_camera, glm::vec3(this->_movementSpeed, 0.0f, 0.0f));    
}

void CameraController::_ascend() {
  moveLocal(this->_camera, glm::vec3(0.0f, this->_movementSpeed, 0.0f));    
}

void CameraController::_descend() {
  moveLocal(this->_camera, glm::vec3(0.0f, -this->_movementSpeed, 0.0f));    
}

void CameraController::_updateMouse(float x, float y) {

}
} // namespace AltheaEngine