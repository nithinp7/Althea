#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace AltheaEngine {
Camera::Camera(float fovDegrees, float aspectRatio, float nearPlane, float farPlane) :
    _transform(1.0f),
    _projection(
      glm::perspective(glm::radians(fovDegrees), aspectRatio, nearPlane, farPlane))
    {
  // Convert from OpenGL to Vulkan screen-Y convention.
  this->_projection[1][1] *= -1.0f;
}

void Camera::setPosition(const glm::vec3& position) {
  this->_transform[3] = glm::vec4(position, 1.0f);
}

void Camera::setRotation(float yawDegrees, float pitchDegrees) {
  pitchDegrees = glm::clamp(pitchDegrees, -89.0f, 89.0f);
  
  float yawRadians = glm::radians(yawDegrees);
  float pitchRadians = glm::radians(pitchDegrees);

  float cosPitch = cos(pitchRadians);
  glm::vec3 zAxis(cos(yawRadians) * cosPitch, -sin(pitchRadians), -sin(yawRadians) * cosPitch);
  glm::vec3 xAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), zAxis));
  glm::vec3 yAxis = glm::cross(zAxis, xAxis);

  // Update only the rotation columns of the transform.
  this->_transform[0] = glm::vec4(xAxis, 0.0f);
  this->_transform[1] = glm::vec4(yAxis, 0.0f);
  this->_transform[2] = glm::vec4(zAxis, 0.0f);
}

float Camera::computeYawDegrees() const {
  const glm::vec4& zAxis = this->_transform[2];
  return glm::degrees(glm::atan(-zAxis.z, zAxis.x));
}

float Camera::computePitchDegrees() const {
  const glm::vec4& zAxis = this->_transform[2];
  
  // The z-axis faces backwards, flip the angle so positive
  // pitch corresponds to the camera tilting up. 
  return -glm::degrees(
      glm::atan(
        zAxis.y, 
        glm::sqrt(
          zAxis.x * zAxis.x + zAxis.z * zAxis.z)));
}

glm::mat4 Camera::computeView() const {
  return glm::affineInverse(this->_transform);
}
} // namespace AltheaEngine