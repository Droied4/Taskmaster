#pragma once
#include "Program.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

class ConfigParser {
private:
  // es el estado de Lua que se usa para cargar la config
  lua_State *L;

  void parse_program_table(const std::string &prog_name, ProgramConfig &cfg);
#ifdef DEBUG
  void
  debug_dump_config(const std::map<std::string, ProgramConfig> &configs) const;
  void dump_stack(lua_State *L);
#endif

public:
  ConfigParser();
  ~ConfigParser();
  std::map<std::string, ProgramConfig> parse(const std::string &filename);
};
