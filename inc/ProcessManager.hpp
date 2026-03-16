#pragma once
#include "Command.hpp"
#include "ConfigParser.hpp"
#include "Process.hpp"
#include "Program.hpp"

class ProcessManager {
private:
  std::string _config_path;
  std::map<std::string, Program *> _programs;
  ConfigParser _parser;

  Start _start_cmd;
  Stop _stop_cmd;
  Status _status_cmd;
  Reload _reload_cmd;
  Restart _restart_cmd;
  Pid _pid_cmd;
  Shutdown _shutdown_cmd;

  Process *findProcessByPid(pid_t pid);
  bool isExpectedExitCode(int exit_code, const std::vector<int> &exitcodes);
  bool shouldRestart(Process *proc, int exit_code);
  void handleProcessRestart(Process *proc);
  Program *findProgramByProcess(Process *proc);
  void handleRestartingProcess(Process *proc, Program *prog);

public:
  ProcessManager(const std::string &config_path);
  ~ProcessManager();
  ProcessManager(const ProcessManager &obj) = delete;
  ProcessManager &operator=(const ProcessManager &obj) = delete;

  void reloadConfig();
  void shutdownAll();

  void reap();
  void updateRunningStates();

  std::string executeCommand(const std::string &cmd,
                             const std::vector<std::string> &params);
};
