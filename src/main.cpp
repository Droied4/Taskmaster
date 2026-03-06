#include "ConfigParser.hpp"

int main() {
  ConfigParser parser;

  std::map<std::string, ProgramConfig> config_map = parser.parse("config.lua");

  return 0;
}
