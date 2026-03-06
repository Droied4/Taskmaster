#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// esto es temporal uwu solo para probar la lectura de la conf hasta que hagamos
// la clase Program
struct ProgramConfig {
  std::string cmd;
  int numprocs;
  mode_t umask;
  std::string workingdir;
  bool autostart;
  std::string autorestart;
  std::vector<int> exitcodes;
  int startretries;
  int starttime;
  int stopsignal;
  int stoptime;
  std::string stdout_path;
  std::string stderr_path;
  std::map<std::string, std::string> env;
};

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
