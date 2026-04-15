#include "ConfigParser.hpp"
#include "Logs.hpp"
#include "common.hpp"
#include <cassert>
#include <csignal>
#include <functional>

ConfigParser::ConfigParser() {
  L = luaL_newstate();
  ASSERT(L != nullptr, "failed to create Lua state: Out of memory");
  luaL_requiref(L, "_G", luaopen_base, 1);
  lua_pop(L, 1);
  luaL_requiref(L, "table", luaopen_table, 1);
  lua_pop(L, 1);
  luaL_requiref(L, "string", luaopen_string, 1);
  lua_pop(L, 1);
}

ConfigParser::~ConfigParser() {
  if (L)
    lua_close(L);
}

bool ConfigParser::parse(const std::string &filename,
                         std::map<std::string, ProgramConfig> &out,
                         std::string &error) {
  out.clear();
#ifdef DEBUG
  dump_stack(L);
#endif
  if (luaL_dofile(L, filename.c_str()) != LUA_OK) {
    std::string err = lua_tostring(L, -1);
    CONFIG_ERROR("Global", "Failed to load config: " + err, error);
  }
  if (!lua_istable(L, -1))
    CONFIG_ERROR("Global", "config.lua did not return a table", error);
  lua_getfield(L, -1, "programs");
  if (!lua_istable(L, -1))
    CONFIG_ERROR("Global",
                 "'programs' field is missing or not a table in config.lua",
                 error);
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (!lua_isstring(L, -2))
      CONFIG_ERROR("Global", "Program name must be a string", error);
    if (!lua_istable(L, -1))
      CONFIG_ERROR(lua_tostring(L, -2), "Program configuration must be a table",
                   error);
    std::string prog_name = lua_tostring(L, -2);
    ProgramConfig cfg;
    Logs::debug() << "Parsing configuration for program: " << prog_name
                  << std::endl;
    if (!parseProgramTable(prog_name, cfg, error))
      return false;
    Logs::debug() << "Finished parsing configuration for program: " << prog_name
                  << std::endl;
    out[prog_name] = cfg;
    lua_pop(L, 1);
  }
  lua_pop(L, 2);
#ifdef DEBUG
  dump_stack(L);
  debug_dump_config(out);
#endif
  Logs::info() << "Configuration parsed successfully with " << out.size()
               << " program(s) defined." << std::endl;
  return true;
}

bool ConfigParser::parseUmask(const std::string &prog_name, ProgramConfig &cfg,
                              std::string &error) {
  if (lua_type(L, -1) != LUA_TSTRING)
    CONFIG_ERROR(prog_name, "'umask' must be a string (e.g., \"022\")", error);
  const char *str = lua_tostring(L, -1);
  char *end;
  long val = strtol(str, &end, 8);
  if (*end != '\0' || val < 0)
    CONFIG_ERROR(prog_name, "Invalid octal format for umask", error);
  cfg.umask = (mode_t)val;
  return true;
}

bool ConfigParser::parseExitCodes(const std::string &prog_name,
                                  ProgramConfig &cfg, std::string &error) {
  cfg.exitcodes.clear();
  if (lua_type(L, -1) == LUA_TNUMBER) {
    cfg.exitcodes.push_back(lua_tointeger(L, -1));
  } else if (lua_istable(L, -1)) {
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
      if (!lua_isnumber(L, -1))
        CONFIG_ERROR(prog_name, "exitcode entry must be a number", error);
      cfg.exitcodes.push_back(lua_tointeger(L, -1));
      lua_pop(L, 1);
    }
  } else {
    CONFIG_ERROR(prog_name, "'exitcodes' must be number or table", error);
  }
  return true;
}

bool ConfigParser::parseEnv(const std::string &prog_name, ProgramConfig &cfg,
                            std::string &error) {
  if (!lua_istable(L, -1))
    CONFIG_ERROR(prog_name, "'env' must be a table", error);
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (!lua_isstring(L, -2))
      CONFIG_ERROR(prog_name, "env key must be string", error);
    if (!lua_isstring(L, -1) && !lua_isnumber(L, -1))
      CONFIG_ERROR(prog_name, "env value must be string or number", error);
    cfg.env[lua_tostring(L, -2)] = lua_tostring(L, -1);
    lua_pop(L, 1);
  }
  return true;
}

bool ConfigParser::parseString(const std::string &key,
                               const std::string &prog_name, std::string &out,
                               std::string &error) {
  if (lua_type(L, -1) != LUA_TSTRING)
    CONFIG_ERROR(prog_name, "'" + key + "' must be a string", error);
  out = lua_tostring(L, -1);
  return true;
}

bool ConfigParser::parseInt(const std::string &key,
                            const std::string &prog_name, int &out,
                            std::string &error) {
  if (lua_type(L, -1) != LUA_TNUMBER)
    CONFIG_ERROR(prog_name, "'" + key + "' must be a number", error);
  out = lua_tointeger(L, -1);
  return true;
}

bool ConfigParser::parseBool(const std::string &key,
                             const std::string &prog_name, bool &out,
                             std::string &error) {
  if (lua_type(L, -1) != LUA_TBOOLEAN)
    CONFIG_ERROR(prog_name, "'" + key + "' must be a boolean", error);
  out = lua_toboolean(L, -1);
  return true;
}

bool ConfigParser::parseAutoRestart(const std::string &prog_name,
                                    ProgramConfig &cfg, std::string &error) {
  if (!parseString("autorestart", prog_name, cfg.autorestart, error))
    return false;
  if (cfg.autorestart != "always" && cfg.autorestart != "never" &&
      cfg.autorestart != "unexpected") {
    CONFIG_ERROR(prog_name,
                 "'autorestart' must be 'always', 'never', or 'unexpected'",
                 error);
  }
  return true;
}

bool ConfigParser::parsePath(const std::string &key,
                             const std::string &prog_name, std::string &out,
                             std::string &error) {
  if (lua_type(L, -1) != LUA_TSTRING)
    CONFIG_ERROR(prog_name, "'" + key + "' must be a string", error);
  out = lua_tostring(L, -1);
  if (out.empty())
    CONFIG_ERROR(prog_name, "'" + key + "' cannot be an empty string", error);
  return true;
}

bool ConfigParser::getSignalFromLua(lua_State *L, const std::string &prog_name,
                                    int &out, std::string &error) {
  int type = lua_type(L, -1);
  if (type == LUA_TNUMBER) {
    int sig = (int)lua_tointeger(L, -1);
    if (sig < 1 || sig > 64)
      CONFIG_ERROR(prog_name, "Signal number out of range (1-64)", error);
    out = sig;
    return true;
  }
  if (type == LUA_TSTRING) {
    static const std::unordered_map<std::string, int> signal_map = {
        {"HUP", SIGHUP},       {"INT", SIGINT},   {"QUIT", SIGQUIT},
        {"ILL", SIGILL},       {"TRAP", SIGTRAP}, {"ABRT", SIGABRT},
        {"BUS", SIGBUS},       {"FPE", SIGFPE},   {"KILL", SIGKILL},
        {"USR1", SIGUSR1},     {"SEGV", SIGSEGV}, {"USR2", SIGUSR2},
        {"PIPE", SIGPIPE},     {"ALRM", SIGALRM}, {"TERM", SIGTERM},
        {"CHLD", SIGCHLD},     {"CONT", SIGCONT}, {"STOP", SIGSTOP},
        {"TSTP", SIGTSTP},     {"TTIN", SIGTTIN}, {"TTOU", SIGTTOU},
        {"URG", SIGURG},       {"XCPU", SIGXCPU}, {"XFSZ", SIGXFSZ},
        {"VTALRM", SIGVTALRM}, {"PROF", SIGPROF}, {"WINCH", SIGWINCH},
        {"IO", SIGIO},         {"PWR", SIGPWR},   {"SYS", SIGSYS}};
    std::string name = lua_tostring(L, -1);
    if (name.substr(0, 3) == "SIG")
      name = name.substr(3);
    auto it = signal_map.find(name);
    if (it != signal_map.end()) {
      out = it->second;
      return true;
    }
    CONFIG_ERROR(prog_name, "Invalid signal name: '" + name + "'", error);
  }
  CONFIG_ERROR(prog_name, "'stopsignal' must be a string or a number", error);
}

const std::unordered_map<std::string, ConfigParser::ParserAction> &
ConfigParser::getHandlers() {
  static const std::unordered_map<std::string, ParserAction> handler = {
      {"cmd",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) { return s->parseString("cmd", n, c.cmd, e); }},
      {"numprocs",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) {
         if (!s->parseInt("numprocs", n, c.numprocs, e))
           return false;
         if (c.numprocs <= 0)
           CONFIG_ERROR(n, "'numprocs' must be positive", e);
         return true;
       }},
      {"workingdir",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) {
         return s->parseString("workingdir", n, c.workingdir, e);
       }},
      {"autostart",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) {
         return s->parseBool("autostart", n, c.autostart, e);
       }},
      {"autorestart",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) { return s->parseAutoRestart(n, c, e); }},
      {"startretries",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) {
         return s->parseInt("startretries", n, c.startretries, e);
       }},
      {"starttime",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) {
         return s->parseInt("starttime", n, c.starttime, e);
       }},
      {"stopsignal",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) {
         return s->getSignalFromLua(s->L, n, c.stopsignal, e);
       }},
      {"stoptime",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) {
         return s->parseInt("stoptime", n, c.stoptime, e);
       }},
      {"stdout",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) {
         return s->parsePath("stdout", n, c.stdout_path, e);
       }},
      {"stderr",
       [](ConfigParser *s, const std::string &n, ProgramConfig &c,
          std::string &e) {
         return s->parsePath("stderr", n, c.stderr_path, e);
       }},
      {"umask", [](ConfigParser *s, const std::string &n, ProgramConfig &c,
                   std::string &e) { return s->parseUmask(n, c, e); }},
      {"exitcodes", [](ConfigParser *s, const std::string &n, ProgramConfig &c,
                       std::string &e) { return s->parseExitCodes(n, c, e); }},
      {"env", [](ConfigParser *s, const std::string &n, ProgramConfig &c,
                 std::string &e) { return s->parseEnv(n, c, e); }}};
  return handler;
}

bool ConfigParser::parseProgramTable(const std::string &prog_name,
                                     ProgramConfig &cfg, std::string &error) {
  ASSERT(lua_istable(L, -1),
         "Value at top of stack is not a table for " + prog_name);
  const auto &handlers = getHandlers();
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (!lua_isstring(L, -2))
      CONFIG_ERROR(prog_name, "Key in program table must be a string", error);
    std::string key = lua_tostring(L, -2);
    if (lua_isnil(L, -1))
      CONFIG_ERROR(prog_name, "Value for key '" + key + "' cannot be nil",
                   error);
    auto it = handlers.find(key);
    if (it == handlers.end())
      CONFIG_ERROR(prog_name, "Unknown key '" + key + "'", error);
    if (!it->second(this, prog_name, cfg, error))
      return false;
    lua_pop(L, 1);
  }
  if (cfg.cmd.empty())
    CONFIG_ERROR(prog_name, "Required field 'cmd' is missing", error);
  return true;
}

#ifdef DEBUG
void ConfigParser::debug_dump_config(
    const std::map<std::string, ProgramConfig> &configs) const {
  std::cout << "\n=======================================================\n";
  std::cout << "             TASKMASTER: FULL CONFIG DUMP               \n";
  std::cout << "=======================================================\n";
  if (configs.empty()) {
    std::cout << "Program table is EMPTY.\n";
    std::cout << "=======================================================\n\n";
    return;
  }
  for (const auto &[name, cfg] : configs) {
    std::cout << "[PROGRAM]: " << name << "\n";
    std::cout << "  - cmd:          " << cfg.cmd << "\n";
    std::cout << "  - numprocs:     " << cfg.numprocs << "\n";
    std::cout << "  - umask:        0" << std::oct << cfg.umask << std::dec
              << "\n";
    std::cout << "  - workingdir:   "
              << (cfg.workingdir.empty() ? "(default)" : cfg.workingdir)
              << "\n";
    std::cout << "  - autostart:    " << (cfg.autostart ? "true" : "false")
              << "\n";
    std::cout << "  - autorestart:  " << cfg.autorestart << "\n";
    std::cout << "  - exitcodes:    [";
    for (size_t i = 0; i < cfg.exitcodes.size(); ++i) {
      std::cout << cfg.exitcodes[i]
                << (i < cfg.exitcodes.size() - 1 ? ", " : "");
    }
    std::cout << "]\n";
    std::cout << "  - startretries: " << cfg.startretries << "\n";
    std::cout << "  - starttime:    " << cfg.starttime << " seg\n";
    std::cout << "  - stopsignal:   " << cfg.stopsignal << "\n";
    std::cout << "  - stoptime:     " << cfg.stoptime << " seg\n";
    std::cout << "  - stdout:       "
              << (cfg.stdout_path.empty() ? "(none)" : cfg.stdout_path) << "\n";
    std::cout << "  - stderr:       "
              << (cfg.stderr_path.empty() ? "(none)" : cfg.stderr_path) << "\n";
    std::cout << "  - env:\n";
    if (cfg.env.empty()) {
      std::cout << "      (none)\n";
    } else {
      for (const auto &[key, val] : cfg.env) {
        std::cout << "      " << key << " = " << val << "\n";
      }
    }
    std::cout << "-------------------------------------------------------\n";
  }
  std::cout << "\n";
}

void ConfigParser::dump_stack(lua_State *L) {
  int top = lua_gettop(L);
  std::cout << "\n=== LUA STACK DUMP (Elements: " << top << ") ===\n";
  for (int i = 1; i <= top; i++) {
    int t = lua_type(L, i);
    std::cout << "[" << i << "] (" << i - top - 1 << "):\t"
              << lua_typename(L, t) << "\t-> ";
    switch (t) {
    case LUA_TSTRING:
      std::cout << "'" << lua_tostring(L, i) << "'";
      break;
    case LUA_TBOOLEAN:
      std::cout << (lua_toboolean(L, i) ? "true" : "false");
      break;
    case LUA_TNUMBER:
      std::cout << lua_tonumber(L, i);
      break;
    default:
      std::cout << lua_topointer(L, i);
      break;
    }
    std::cout << "\n";
  }
  std::cout << "=======================================\n\n";
}
#endif
