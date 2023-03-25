#include "Camera.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace AltheaEngine {
Camera::Camera(
    float fovDegrees,
    float aspectRatio,
    float nearPlane,
    float farPlane)
    : _fov(glm::radians(fovDegrees)),
      _aspectRatio(aspectRatio),
      _nearPlane(nearPlane),
      _farPlane(farPlane),
      _transform(1.0f),
      _projection(1.0f) {
  this->_recomputeProjection();
}

void Camera::setFovDegrees(float fovDegrees) {
  this->_fov = glm::radians(fovDegrees);
  this->_recomputeProjection();
}

void Camera::setAspectRatio(float aspectRatio) {
  this->_aspectRatio = aspectRatio;
  this->_recomputeProjection();
}

void Camera::setClippingPlanes(float nearPlane, float farPlane) {
  this->_nearPlane = nearPlane;
  this->_farPlane = farPlane;

  this->_recomputeProjection();
}

void Camera::setPosition(const glm::vec3& position) {
  this->_transform[3] = glm::vec4(position, 1.0f);
}

void Camera::setRotation(float yawDegrees, float pitchDegrees) {
  pitchDegrees = glm::clamp(pitchDegrees, -89.0f, 89.0f);

  float yawRadians = glm::radians(yawDegrees);
  float pitchRadians = glm::radians(pitchDegrees);

  float cosPitch = cos(pitchRadians);
  glm::vec3 zAxis(
      sin(yawRadians) * cosPitch,
      -sin(pitchRadians),
      cos(yawRadians) * cosPitch);
  glm::vec3 xAxis =
      glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), zAxis));
  glm::vec3 yAxis = glm::cross(zAxis, xAxis);

  // Update only the rotation columns of the transform.
  this->_transform[0] = glm::vec4(xAxis, 0.0f);
  this->_transform[1] = glm::vec4(yAxis, 0.0f);
  this->_transform[2] = glm::vec4(zAxis, 0.0f);
}

// TODO: Double check these
float Camera::computeYawDegrees() const {
  const glm::vec4& zAxis = this->_transform[2];
  return glm::degrees(glm::atan(-zAxis.z, zAxis.x));
}

float Camera::computePitchDegrees() const {
  const glm::vec4& zAxis = this->_transform[2];

  // The z-axis faces backwards, flip the angle so positive
  // pitch corresponds to the camera tilting up.
  return -glm::degrees(
      glm::atan(zAxis.y, glm::sqrt(zAxis.x * zAxis.x + zAxis.z * zAxis.z)));
}

glm::mat4 Camera::computeView() const {
  return glm::affineInverse(this->_transform);
}

void Camera::_recomputeProjection() {
  this->_projection = glm::perspective(
      this->_fov,
      this->_aspectRatio,
      this->_nearPlane,
      this->_farPlane);

  // Convert from OpenGL to Vulkan screen-Y convention.
  this->_projection[1][1] *= -1.0f;
}
} // namespace AltheaEngine