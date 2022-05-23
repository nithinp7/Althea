#pragma once

#include "ConfigParser.h"

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include <CesiumGltf/Model.h>

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};

class Model {
public:
  Model(const std::string& path);

private:
  CesiumGltf::Model model;
  std::vector<Vertex> vertices;
};

class ModelManager {
  std::vector<Model> models;
public:
  ModelManager(const ConfigParser& configParser);
};

