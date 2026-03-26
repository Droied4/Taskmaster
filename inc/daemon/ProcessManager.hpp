#pragma once

#include "Command.hpp"
#include "ConfigParser.hpp"
#include <memory>
#include <string>

class ProcessManager {
private:
  std::string _config_path;
  std::map<std::string, std::unique_ptr<Program>> _programs;
  std::vector<std::unique_ptr<Program>> _graveyard;
  ConfigParser _parser;

  std::map<std::string, std::unique_ptr<Command>> _commands;

  Process *findProcessByPid(pid_t pid);
  bool isExpectedExitCode(int exit_code, const std::vector<int> &exitcodes);
  bool shouldRestart(Process *proc, int exit_code);
  void handleProcessRestart(Process *proc);
  Program *findProgramByProcess(Process *proc);
  void handleRestartingProcess(Process *proc, Program *prog);

  std::string getPrograms();

public:
  ProcessManager(const std::string &config_path);
  ~ProcessManager();
  ProcessManager(const ProcessManager &obj) = delete;
  ProcessManager &operator=(const ProcessManager &obj) = delete;

  void reloadConfig();
  void shutdownAll();
  void startAutostart();

  void reap();
  void updateRunningStates();

  std::string executeCommand(const std::string &cmd,
                             const std::vector<std::string> &params);
  std::string getCommands();

  Process *getExactProcess(const std::string &target);

  bool hasActiveProcesses() const;
};
