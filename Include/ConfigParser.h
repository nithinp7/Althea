#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

namespace AltheaEngine {
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
} // namespace AltheaEngine


