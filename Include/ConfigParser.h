#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

struct ShaderConfig {
  std::string shaderPath;
  std::string compiledShaderOutputPath;
};

struct ConfigParseResult {
  // TODO: avoid having to recompile shaders every time
  // The paths of shaders to load and compile.
  // SHADER_LIST
  // <shader path> <compiled shader output dir>
  // ...
  std::vector<ShaderConfig> shaderList;
    
  // Named shader pipelines, specified as lists of indices
  // corresponding to shaders in the shaderList.
  // SHADER_PIPELINES
  // <pipeline name> <shader Id> <shader Id> ...
  // ...
  std::map<std::string, std::vector<uint32_t>> shaderPipelines;
};

class ConfigCategory {
public:
  ConfigCategory(const std::string& name);

private:
  friend class ConfigParser;

  std::string _name;

protected:
  virtual void parseLine(const std::string& line) = 0;
};

class ConfigParser
{
public:
  ConfigParser(const std::string& configFilePath);
  void parseCategory(ConfigCategory& category) const;

private:
  std::vector<std::string> _lines;
};


