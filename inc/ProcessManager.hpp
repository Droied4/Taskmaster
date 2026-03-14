#pragma once
#include "Command.hpp"
#include "ConfigParser.hpp"
#include "Process.hpp"
#include "Program.hpp"

class ProcessManager {
private:
  std::map<std::string, Program *> _programs;
  ConfigParser _parser;

  Start _start_cmd;
  Stop _stop_cmd;
  Status _status_cmd;
  Reload _reload_cmd;
  Restart _restart_cmd;
  Shutdown _shutdown_cmd;

  Process *findProcessByPid(pid_t pid);

public:
  ProcessManager(const std::map<std::string, ProgramConfig> &configs);
  ~ProcessManager();
  ProcessManager(const ProcessManager &obj) = delete;
  ProcessManager &operator=(const ProcessManager &obj) = delete;

  void reloadConfig();
  void shutdownAll();

  std::string executeCommand(const std::string &cmd,
                             const std::vector<std::string> &params);
};
