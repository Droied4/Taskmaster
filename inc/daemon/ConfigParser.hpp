#pragma once
#include "Program.hpp"
#include <functional>
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
class ConfigParser {
private:
  lua_State *L;
  using ParserAction =
      std::function<bool(ConfigParser *self, const std::string &prog_name,
                         ProgramConfig &cfg, std::string &error)>;
  bool parseProgramTable(const std::string &prog_name, ProgramConfig &cfg,
                         std::string &error);
  bool parseUmask(const std::string &prog_name, ProgramConfig &cfg,
                  std::string &error);
  bool parseExitCodes(const std::string &prog_name, ProgramConfig &cfg,
                      std::string &error);
  bool parseEnv(const std::string &prog_name, ProgramConfig &cfg,
                std::string &error);
  bool parseString(const std::string &key, const std::string &prog_name,
                   std::string &out, std::string &error);
  bool parseInt(const std::string &key, const std::string &prog_name, int &out,
                std::string &error);
  bool parseBool(const std::string &key, const std::string &prog_name,
                 bool &out, std::string &error);
  bool parseAutoRestart(const std::string &prog_name, ProgramConfig &cfg,
                        std::string &error);
  bool getSignalFromLua(lua_State *L, const std::string &prog_name, int &out,
                        std::string &error);
  bool parsePath(const std::string &key, const std::string &prog_name,
                 std::string &out, std::string &error);
  const std::unordered_map<std::string, ParserAction> &getHandlers();
#ifdef DEBUG
  void
  debug_dump_config(const std::map<std::string, ProgramConfig> &configs) const;
  void dump_stack(lua_State *L);
#endif
public:
  ConfigParser();
  ~ConfigParser();
  ConfigParser(const ConfigParser &) = delete;
  ConfigParser &operator=(const ConfigParser &) = delete;
  bool parse(const std::string &filename,
             std::map<std::string, ProgramConfig> &out, std::string &error);
};
