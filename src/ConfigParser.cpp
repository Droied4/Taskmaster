#include "ConfigParser.hpp"
#include "common.hpp"
#include <cassert>
#include <csignal>
#include <functional>

static int getSignalFromLua(lua_State *L, const std::string &prog_name);

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

std::map<std::string, ProgramConfig>
ConfigParser::parse(const std::string &filename) {
#ifdef DEBUG
  dump_stack(L);
#endif
  std::map<std::string, ProgramConfig> parsed_configs;

  if (luaL_dofile(L, filename.c_str()) != LUA_OK) {
    std::string err = lua_tostring(L, -1);
    CONFIG_ERROR("Global", "Failed to load config: " + err);
  }

  if (!lua_istable(L, -1))
    CONFIG_ERROR("Global", "config.lua did not return a table");

  lua_getfield(L, -1, "programs");
  if (!lua_istable(L, -1))
    CONFIG_ERROR("Global",
                 "'programs' field is missing or not a table in config.lua");

  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (!lua_isstring(L, -2))
      CONFIG_ERROR("Global", "Program name must be a string");
    if (!lua_istable(L, -1))
      CONFIG_ERROR(lua_tostring(L, -2),
                   "Program configuration must be a table");

    std::string prog_name = lua_tostring(L, -2);
    ProgramConfig cfg;
    parseProgramTable(prog_name, cfg);
    parsed_configs[prog_name] = cfg;

    lua_pop(L, 1);
  }

  lua_pop(L, 2);

#ifdef DEBUG
  dump_stack(L);
  debug_dump_config(parsed_configs);
#endif
  return parsed_configs;
}

void ConfigParser::parseUmask(const std::string &prog_name,
                              ProgramConfig &cfg) {
  if (lua_type(L, -1) != LUA_TSTRING)
    CONFIG_ERROR(prog_name, "'umask' must be a string (e.g., \"022\")");
  try {
    cfg.umask = std::stoi(lua_tostring(L, -1), nullptr, 8);
  } catch (...) {
    CONFIG_ERROR(prog_name, "Invalid octal format for umask");
  }
}

void ConfigParser::parseExitCodes(const std::string &prog_name,
                                  ProgramConfig &cfg) {
  cfg.exitcodes.clear();
  if (lua_type(L, -1) == LUA_TNUMBER) {
    cfg.exitcodes.push_back(lua_tointeger(L, -1));
  } else if (lua_istable(L, -1)) {
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
      if (!lua_isnumber(L, -1))
        CONFIG_ERROR(prog_name, "exitcode entry must be a number");
      cfg.exitcodes.push_back(lua_tointeger(L, -1));
      lua_pop(L, 1);
    }
  } else {
    CONFIG_ERROR(prog_name, "'exitcodes' must be number or table");
  }
}

void ConfigParser::parseEnv(const std::string &prog_name, ProgramConfig &cfg) {
  if (!lua_istable(L, -1))
    CONFIG_ERROR(prog_name, "'env' must be a table");
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (!lua_isstring(L, -2))
      CONFIG_ERROR(prog_name, "env key must be string");
    if (!lua_isstring(L, -1) && !lua_isnumber(L, -1))
      CONFIG_ERROR(prog_name, "env value must be string or number");
    cfg.env[lua_tostring(L, -2)] = lua_tostring(L, -1);
    lua_pop(L, 1);
  }
}

void ConfigParser::parseProgramTable(const std::string &prog_name,
                                     ProgramConfig &cfg) {
  ASSERT(lua_istable(L, -1),
         "Value at top of stack is not a table for " + prog_name);

  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (!lua_isstring(L, -2))
      CONFIG_ERROR(prog_name, "Key in program table must be a string");
    std::string key = lua_tostring(L, -2);

    if (lua_isnil(L, -1))
      CONFIG_ERROR(prog_name, "Value for key '" + key + "' cannot be nil");

    setConfig(key, prog_name, cfg);

    lua_pop(L, 1);
  }

  if (cfg.cmd.empty()) {
    CONFIG_ERROR(prog_name, "Required field 'cmd' is missing or set to nil.");
  }
}

void ConfigParser::setConfig(const std::string &key,
                             const std::string &prog_name, ProgramConfig &cfg) {
  auto checkString = [&](const std::string &key) {
    if (lua_type(L, -1) != LUA_TSTRING)
      CONFIG_ERROR(prog_name, "'" + key + "' must be a string");
    return std::string(lua_tostring(L, -1));
  };

  auto checkInt = [&](const std::string &key) {
    if (lua_type(L, -1) != LUA_TNUMBER)
      CONFIG_ERROR(prog_name, "'" + key + "' must be a number");
    return static_cast<int>(lua_tointeger(L, -1));
  };

  auto checkBool = [&](const std::string &key) {
    if (lua_type(L, -1) != LUA_TBOOLEAN)
      CONFIG_ERROR(prog_name, "'" + key + "' must be a boolean");
    return static_cast<bool>(lua_toboolean(L, -1));
  };

  if (key == "cmd")
    cfg.cmd = checkString(key);
  else if (key == "numprocs") {
    cfg.numprocs = checkInt(key);
    if (cfg.numprocs <= 0)
      CONFIG_ERROR(prog_name, "numprocs must be positive");
  } else if (key == "workingdir")
    cfg.workingdir = checkString(key);
  else if (key == "autostart")
    cfg.autostart = checkBool(key);
  else if (key == "autorestart") {
    cfg.autorestart = checkString(key);
    if (cfg.autorestart != "always" && cfg.autorestart != "never" &&
        cfg.autorestart != "unexpected") {
      CONFIG_ERROR(prog_name,
                   "'autorestart' must be 'always', 'never', or 'unexpected'");
    }

  } else if (key == "startretries")
    cfg.startretries = checkInt(key);
  else if (key == "starttime")
    cfg.starttime = checkInt(key);
  else if (key == "stopsignal")
    cfg.stopsignal = getSignalFromLua(L, prog_name);
  else if (key == "stoptime")
    cfg.stoptime = checkInt(key);
  else if (key == "stdout")
    cfg.stdout_path = checkString(key);
  else if (key == "stderr")
    cfg.stderr_path = checkString(key);
  else if (key == "umask")
    parseUmask(prog_name, cfg);
  else if (key == "exitcodes")
    parseExitCodes(prog_name, cfg);
  else if (key == "env")
    parseEnv(prog_name, cfg);
  else
    CONFIG_ERROR(prog_name, "Unknown key '" + key + "'");
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

static int getSignalFromLua(lua_State *L, const std::string &prog_name) {
  int type = lua_type(L, -1);

  if (type == LUA_TNUMBER) {
    int sig = (int)lua_tointeger(L, -1);
    if (sig < 1 || sig > 64) {
      CONFIG_ERROR(prog_name, "Signal number out of range (1-64)");
    }
    return sig;
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
    if (it != signal_map.end())
      return it->second;

    CONFIG_ERROR(prog_name, "Invalid signal name: '" + name + "'");
  }

  CONFIG_ERROR(prog_name, "'stopsignal' must be a string or a number");
  return -1;
}
