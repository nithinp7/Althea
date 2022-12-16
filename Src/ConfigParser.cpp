#include "ConfigParser.h"

#include <fstream>
#include <utility>

namespace AltheaEngine {
ConfigCategory::ConfigCategory(
    const std::string& name) 
  : _name(name)  {}
  
ConfigParser::ConfigParser(const std::string& configFilePath) {
  std::ifstream infile(configFilePath);
  std::string line;
  while (std::getline(infile, line)) {
    this->_lines.push_back(line);
  }
}

void ConfigParser::parseCategory(ConfigCategory& category) const {
  for (auto lineIt = this->_lines.begin(); 
       lineIt != this->_lines.end(); 
       ++lineIt) {
    if (category._name == *lineIt) {
      ++lineIt;
      for (; lineIt != this->_lines.end() && *lineIt != ""; ++lineIt) {
        category.parseLine(*lineIt);
      }

      break;
    }
  }
}
} // namespace AltheaEngine
