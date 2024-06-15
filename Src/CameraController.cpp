#include "CameraController.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>

#include <functional>

namespace AltheaEngine {
CameraController::CameraController(
    InputManager& inputManager,
    float fovDegrees,
    float aspectRatio)
    : _camera(fovDegrees, aspectRatio, 0.01f, 1000.0f) {
  // TODO: allow bindings from config file

  // Bind inputs.
  inputManager.addKeyBinding(
      {GLFW_KEY_W, GLFW_PRESS, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 2, -1));
  inputManager.addKeyBinding(
      {GLFW_KEY_W, GLFW_RELEASE, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 2, 0));
  inputManager.addKeyBinding(
      {GLFW_KEY_A, GLFW_PRESS, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 0, -1));
  inputManager.addKeyBinding(
      {GLFW_KEY_A, GLFW_RELEASE, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 0, 0));
  inputManager.addKeyBinding(
      {GLFW_KEY_S, GLFW_PRESS, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 2, 1));
  inputManager.addKeyBinding(
      {GLFW_KEY_S, GLFW_RELEASE, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 2, 0));
  inputManager.addKeyBinding(
      {GLFW_KEY_D, GLFW_PRESS, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 0, 1));
  inputManager.addKeyBinding(
      {GLFW_KEY_D, GLFW_RELEASE, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 0, 0));
  inputManager.addKeyBinding(
      {GLFW_KEY_Q, GLFW_PRESS, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 1, -1));
  inputManager.addKeyBinding(
      {GLFW_KEY_Q, GLFW_RELEASE, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 1, 0));
  inputManager.addKeyBinding(
      {GLFW_KEY_E, GLFW_PRESS, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 1, 1));
  inputManager.addKeyBinding(
      {GLFW_KEY_E, GLFW_RELEASE, 0},
      std::bind(&CameraController::_updateTargetDirection, this, 1, 0));

  inputManager.addKeyBinding(
      {GLFW_KEY_X, GLFW_PRESS, 0},
      [&acceleration = this->_acceleration]() { acceleration = 4.0f; });
  inputManager.addKeyBinding(
      {GLFW_KEY_X, GLFW_RELEASE, 0},
      [&acceleration = this->_acceleration]() { acceleration = 0.0f; });

  inputManager.addKeyBinding(
      {GLFW_KEY_Z, GLFW_PRESS, 0},
      [&acceleration = this->_acceleration]() { acceleration = -6.0f; });
  inputManager.addKeyBinding(
      {GLFW_KEY_Z, GLFW_RELEASE, 0},
      [&acceleration = this->_acceleration]() { acceleration = 0.0f; });

  inputManager.addMousePositionCallback(
      [this](double x, double y, bool cursorHidden) {
        this->_updateMouse(x, y, cursorHidden);
      });
}

CameraController::~CameraController() {
  // TODO: unbind?
}

void CameraController::tick(float deltaTime) {
  // Cap max _perceived_ deltaTime, to avoid strange behavior doing
  // lag spikes (e.g., renderstate recreation on window resize)
  deltaTime = glm::clamp(deltaTime, 0.0f, 1.0f / 30.0f);

  this->_targetSpeed = glm::clamp(
      this->_targetSpeed + this->_acceleration * deltaTime,
      this->_minSpeed,
      this->_maxSpeed);

  const glm::mat4& transform = this->_camera.getTransform();
  glm::vec3 position(transform[3]);

  float pitch = this->_camera.computePitchDegrees();

  glm::vec3 targetVelocity;
  float directionMagnitude = glm::length(this->_targetDirection);
  if (directionMagnitude < 0.001f) {
    targetVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
  } else {
    glm::vec3 targetVelocityLocal =
        this->_targetSpeed * glm::normalize(this->_targetDirection);
    targetVelocity =
        glm::vec3(transform * glm::vec4(targetVelocityLocal, 0.0f));
  }

  // Velocity feedback controller
  float K = 4.0f / this->_velocitySettleTime;
  glm::vec3 acceleration = K * (targetVelocity - this->_velocity);

  // Orientation feedback controller
  float wn = 4.0f / this->_orientationSettleTime;
  float Kv = 2.0f * wn;
  float Kp = wn * wn;

  float pitchDiff = this->_targetPitch - pitch;
  float yawDiff = this->_targetYaw - this->_yaw;

  float pitchAcceleration = Kp * pitchDiff - Kv * this->_pitchRate;
  float yawAcceleration = Kp * yawDiff - Kv * this->_yawRate;

  // Euler integration step
  this->_velocity += acceleration * deltaTime;
  glm::vec3 newPosition = position + this->_velocity * deltaTime;

  this->_pitchRate += pitchAcceleration * deltaTime;
  this->_yawRate += yawAcceleration * deltaTime;
  float newPitch = pitch + this->_pitchRate * deltaTime;
  this->_yaw += this->_yawRate * deltaTime;

  this->_camera.setPosition(newPosition);
  this->_camera.setRotationDegrees(this->_yaw, newPitch);
}

void CameraController::setMinSpeed(float speed) { this->_minSpeed = speed; }

void CameraController::setMaxSpeed(float speed) { this->_maxSpeed = speed; }

void CameraController::setPosition(const glm::vec3& position) {
  this->_camera.setPosition(position);
}

void CameraController::setRotationDegrees(
    float yawDegrees,
    float pitchDegrees) {
  this->_camera.setRotationDegrees(yawDegrees, pitchDegrees);
  this->_yaw = yawDegrees;
  this->_targetYaw = yawDegrees;
}

void CameraController::_updateTargetDirection(uint32_t axis, int dir) {
  this->_targetDirection[axis] = static_cast<float>(dir);
}

static void moveLocal(Camera& camera, const glm::vec3& localDisplacement) {
  const glm::mat4& transform = camera.getTransform();

  glm::vec3 newPos(
      transform[3] + transform * glm::vec4(localDisplacement, 0.0f));
  camera.setPosition(newPos);
}

void CameraController::_updateMouse(double x, double y, bool cursorHidden) {
  if (cursorHidden) {
    float unclampedTargetYaw =
        this->_yawMultiplier * static_cast<float>(-180.0 * x);
    float unclampedTargetPitch =
        this->_pitchMultiplier * static_cast<float>(89.0 * y);

    if (this->_mouseDisabled) {
      // The mouse is just now becoming enabled, offset the target pitch and
      // yaw to avoid spurious movement.
      this->_targetYawOffset = unclampedTargetYaw - this->_targetYaw;
      this->_targetPitchOffset = unclampedTargetPitch - this->_targetPitch;

      this->_mouseDisabled = false;
    }

    // Yaw increases to the left
    // Clamp the change in yaw so it never reaches a half-turn or more,
    // otherwise large mouse motions will cause the camera to turn in the
    // "wrong" direction.
    float targetYawChange = glm::clamp(
        unclampedTargetYaw - (this->_targetYaw + this->_targetYawOffset),
        -179.0f,
        179.0f);
    this->_targetYaw += targetYawChange;

    // Clamp the pitch to avoid disorientation and gimbal lock.
    this->_targetPitch = glm::clamp(
        unclampedTargetPitch - this->_targetPitchOffset,
        -89.0f,
        89.0f);
  } else {
    this->_mouseDisabled = true;
  }
}
} // namespace AltheaEngine