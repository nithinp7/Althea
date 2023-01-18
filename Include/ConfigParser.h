#pragma once

#include "Library.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


namespace AltheaEngine {
class ALTHEA_API ConfigCategory {
public:
  ConfigCategory(const std::string& name);

private:
  friend class ConfigParser;

  std::string _name;

protected:
  virtual void parseLine(const std::string& line) = 0;
};

class ALTHEA_API ConfigParser {
public:
  ConfigParser(const std::string& configFilePath);
  void parseCategory(ConfigCategory& category) const;

private:
  std::vector<std::string> _lines;
};
} // namespace AltheaEngine
