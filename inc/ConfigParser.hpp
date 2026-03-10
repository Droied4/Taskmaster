#pragma once
#include "Program.hpp"
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
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

  using ParserAction = void (*)(ConfigParser *self,
                                const std::string &prog_name,
                                ProgramConfig &cfg);

  void parseProgramTable(const std::string &prog_name, ProgramConfig &cfg);
  void parseUmask(const std::string &prog_name, ProgramConfig &cfg);
  void parseExitCodes(const std::string &prog_name, ProgramConfig &cfg);
  void parseEnv(const std::string &prog_name, ProgramConfig &cfg);
  void parseString(const std::string &key, const std::string &prog_name,
                   std::string &out);
  void parseInt(const std::string &key, const std::string &prog_name, int &out,
                bool check_pos = false);
  void parseBool(const std::string &key, const std::string &prog_name,
                 bool &out);
  void parseAutoRestart(const std::string &prog_name, ProgramConfig &cfg);
  int getSignalFromLua(lua_State *L, const std::string &prog_name);

  static const std::unordered_map<std::string, ParserAction> &getHandlers();

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
