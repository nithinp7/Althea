#include "Camera.h"
#include <glm/gtc/matrix_inverse.hpp>

namespace AltheaEngine {
Camera::Camera(float fovDegrees, float aspectRatio, float nearPlane, float farPlane) :
    _transform(1.0f),
    _projection(
      glm::perspective(glm::radians(fovDegrees), aspectRatio, nearPlane, farPlane))
    {}

void Camera::tick(float deltaTime) {}

void Camera::setPosition(const glm::vec3& position) {
  this->_transform[3] = position;
}

void Camera::setRotation(float yawDegrees, float pitchDegrees) {
  pitchDegrees = glm::clamp(pitchDegrees, -89.0f, 89.0f);
  glm::mat4 R = 
      glm::rotate(glm::radians(yawDegrees), glm::vec3(0.0f, 1.0f, 0.0f)) *
      glm::rotate(glm::radians(pitchDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
  
  // Update only the rotation columns of the transform.
  this->_transform[0] = R[0];
  this->_transform[1] = R[1];
  this->_transform[2] = R[2];
}

glm::mat4 Camera::computeView() const {
  return glm::affineInverse(this->_transform);
}
} // namespace AltheaEngine