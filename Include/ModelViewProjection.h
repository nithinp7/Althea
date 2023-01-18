#pragma once

#include "Library.h"

#include <glm/mat4x4.hpp>

namespace AltheaEngine {
struct ALTHEA_API ModelViewProjection {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
};
} // namespace AltheaEngine
