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
  inputManager.addBinding({GLFW_KEY_W, GLFW_PRESS, 0}, std::bind(&CameraController::_updateTargetDirection, this, 2, -1));
  inputManager.addBinding({GLFW_KEY_W, GLFW_RELEASE, 0}, std::bind(&CameraController::_updateTargetDirection, this, 2, 0));
  inputManager.addBinding({GLFW_KEY_A, GLFW_PRESS, 0}, std::bind(&CameraController::_updateTargetDirection, this, 0, -1));
  inputManager.addBinding({GLFW_KEY_A, GLFW_RELEASE, 0}, std::bind(&CameraController::_updateTargetDirection, this, 0, 0));
  inputManager.addBinding({GLFW_KEY_S, GLFW_PRESS, 0}, std::bind(&CameraController::_updateTargetDirection, this, 2, 1));
  inputManager.addBinding({GLFW_KEY_S, GLFW_RELEASE, 0}, std::bind(&CameraController::_updateTargetDirection, this, 2, 0));
  inputManager.addBinding({GLFW_KEY_D, GLFW_PRESS, 0}, std::bind(&CameraController::_updateTargetDirection, this, 0, 1));
  inputManager.addBinding({GLFW_KEY_D, GLFW_RELEASE, 0}, std::bind(&CameraController::_updateTargetDirection, this, 0, 0));
  inputManager.addBinding({GLFW_KEY_Q, GLFW_PRESS, 0}, std::bind(&CameraController::_updateTargetDirection, this, 1, -1));
  inputManager.addBinding({GLFW_KEY_Q, GLFW_RELEASE, 0}, std::bind(&CameraController::_updateTargetDirection, this, 1, 0));
  inputManager.addBinding({GLFW_KEY_E, GLFW_PRESS, 0}, std::bind(&CameraController::_updateTargetDirection, this, 1, 1));
  inputManager.addBinding({GLFW_KEY_E, GLFW_RELEASE, 0}, std::bind(&CameraController::_updateTargetDirection, this, 1, 0));
}

CameraController::~CameraController() {
  // TODO: unbind?
}

void CameraController::tick(float deltaTime) {
  const glm::mat4& transform = this->_camera.getTransform();
  glm::vec3 position(transform[3]);

  glm::vec3 targetVelocity;
  float directionMagnitude = glm::length(this->_targetDirection);
  if (directionMagnitude < 0.001f) {
    targetVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
  } else {
    glm::vec3 targetVelocityLocal = this->_targetSpeed * glm::normalize(this->_targetDirection);
    targetVelocity = glm::vec3(transform * glm::vec4(targetVelocityLocal, 0.0f));
  }
  
  // Feedback controller
  float K = 4.0f / this->_velocitySettleTime;
  glm::vec3 acceleration = K * (targetVelocity - this->_velocity);

  // Euler integration step
  this->_velocity += acceleration * deltaTime;
  glm::vec3 newPosition = position + this->_velocity * deltaTime;

  this->_camera.setPosition(newPosition);
}

void CameraController::_updateTargetDirection(uint32_t axis, int dir) {
  this->_targetDirection[axis] = static_cast<float>(dir);
}

static void moveLocal(Camera& camera, const glm::vec3& localDisplacement) {
  const glm::mat4& transform = camera.getTransform();

  glm::vec3 newPos(transform[3] + transform * glm::vec4(localDisplacement, 0.0f));
  camera.setPosition(newPos);
}

void CameraController::_updateMouse(float x, float y) {

}
} // namespace AltheaEngine