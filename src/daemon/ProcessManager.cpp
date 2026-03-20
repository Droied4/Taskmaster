#include "ProcessManager.hpp"
#include "Logs.hpp"
#include "Process.hpp"
#include "common.hpp"
#include <algorithm>
#include <set>
#include <sys/wait.h>

ProcessManager::ProcessManager(const std::string &config_path)
    : _config_path(config_path) {
  ASSERT(!_config_path.empty(), "Config path cannot be empty");
  std::map<std::string, ProgramConfig> configs;
  std::string error;
  if (!_parser.parse(_config_path, configs, error)) {
    Logs::error() << error << "\n";
    std::exit(1);
  }
  for (const auto &[name, config] : configs) {
    _programs[name] = std::make_unique<Program>(name, config);
    if (config.autostart) {
      _programs[name]->start();
    }
  }
}

ProcessManager::~ProcessManager() {
  _programs.clear();
  _graveyard.clear();
}

Process *ProcessManager::findProcessByPid(pid_t pid) {
  for (const auto &[name, prog] : _programs) {
    for (Process *proc : prog->getProcesses()) {
      if (proc->getPid() == pid) {
        return proc;
      }
    }
  }

  for (const auto &prog : _graveyard) {
    for (Process *proc : prog->getProcesses()) {
      if (proc->getPid() == pid) {
        return proc;
      }
    }
  }
  return nullptr;
}

Program *ProcessManager::findProgramByProcess(Process *proc) {
  for (const auto &[name, prog] : _programs) {
    for (Process *p : prog->getProcesses()) {
      if (p == proc)
        return prog.get();
    }
  }
  return nullptr;
}

std::string
ProcessManager::executeCommand(const std::string &cmd,
                               const std::vector<std::string> &params) {
  ASSERT(!cmd.empty(), "Command cannot be empty");

  if (cmd == "reload")
    return _reload_cmd.execute(_programs, "");
  if (cmd == "shutdown")
    return _shutdown_cmd.execute(_programs, "");
  if (cmd == "_get_programs")
    return getPrograms();

  static const std::map<std::string, Command *> command_map = {
      {"start", &_start_cmd},     {"stop", &_stop_cmd},
      {"restart", &_restart_cmd}, {"status", &_status_cmd},
      {"pid", &_pid_cmd},
  };

  static const std::set<std::string> not_target_commands = {"status", "pid"};

  auto it = command_map.find(cmd);
  if (it == command_map.end()) {
    return "Error: Unknown command: " + cmd + "\n";
  }

  if (params.empty()) {
    if (not_target_commands.count(cmd))
      return it->second->execute(_programs, "");
    return "Error: Command '" + cmd + "' requires a target\n";
  }

  std::string response;
  for (const std::string &target : params)
    response += it->second->execute(_programs, target);

  return response;
}

std::string ProcessManager::getPrograms() {
  std::string result;
  for (const auto &[name, prog] : _programs) {
    result += name + "\n";
    if (prog->getConfig().numprocs > 1) {
      result += name + ":*\n";
      for (Process *proc : prog->getProcesses())
        result += name + ":" + proc->getName() + "\n";
    }
  }
  return result;
}

void ProcessManager::reloadConfig() {
  Logs::info() << "[ProcessManager] Reloading config from: " << _config_path
               << "\n";
  std::map<std::string, ProgramConfig> new_configs;

  std::string error;
  if (!_parser.parse(_config_path, new_configs, error)) {
    Logs::error() << error << "\n";
    Logs::warning() << "Keeping existing configuration and running processes\n";
    return;
  }

  for (auto it = _programs.begin(); it != _programs.end();) {
    if (new_configs.find(it->first) == new_configs.end()) {
      it->second->stop();
      _graveyard.push_back(std::move(it->second));
      Logs::info() << it->first
                   << " removed from config, stopping and removing from active "
                      "programs\n";
      it = _programs.erase(it);
    } else {
      ++it;
    }
  }

  for (const auto &[name, new_cfg] : new_configs) {
    auto it = _programs.find(name);
    if (it == _programs.end()) {
      _programs[name] = std::make_unique<Program>(name, new_cfg);
      Logs::info() << name << " added to config, creating new program\n";
      if (new_cfg.autostart)
        _programs[name]->start();
    } else {
      if (it->second->getConfig() != new_cfg) {
        Logs::info() << name << " configuration changed, restarting program\n";
        it->second->stop();
        _graveyard.push_back(std::move(it->second));
        _programs[name] = std::make_unique<Program>(name, new_cfg);
        _programs[name]->start();
      }
    }
  }
}

void ProcessManager::shutdownAll() {
  for (const auto &[name, prog] : _programs)
    prog->stop();
}

bool ProcessManager::isExpectedExitCode(int exit_code,
                                        const std::vector<int> &exitcodes) {
  return std::find(exitcodes.begin(), exitcodes.end(), exit_code) !=
         exitcodes.end();
}

bool ProcessManager::shouldRestart(Process *proc, int exit_code) {
  ASSERT(proc != nullptr, "Process cannot be null in shouldRestart");
  const std::string &autorestart = proc->getConfig().autorestart;
  if (autorestart == "never")
    return false;
  if (autorestart == "always")
    return true;
  return !isExpectedExitCode(exit_code, proc->getConfig().exitcodes);
}

void ProcessManager::handleProcessRestart(Process *proc) {
  ASSERT(proc != nullptr, "Process cannot be null in handleProcessRestart");
  const ProgramConfig &config = proc->getConfig();
  if (proc->getRetries() >= config.startretries) {
    Logs::warning() << "[ProcessManager] " << proc->getName()
                    << " reached max retries (" << config.startretries
                    << "), marking as FATAL\n";
    proc->setState(ProcessState::FATAL);
    proc->setEndTime(time(NULL));
    proc->setStatusMsg("Exited too quickly (process log may have details)");
    return;
  }
  proc->incrementRetries();
  proc->setState(ProcessState::BACKOFF);
  proc->setStatusMsg("Restarting...");
  Logs::info() << "[ProcessManager] Restarting " << proc->getName()
               << " (retry " << proc->getRetries() << "/" << config.startretries
               << ")\n";
  proc->spawn();
}

void ProcessManager::handleRestartingProcess(Process *proc, Program *prog) {
  proc->setState(ProcessState::STOPPED);
  proc->setEndTime(time(NULL));
  proc->setStatusMsg("Stopped");

  bool all_stopped = true;
  for (Process *p : prog->getProcesses()) {
    ProcessState s = p->getState();
    if (s != ProcessState::STOPPED && s != ProcessState::EXITED &&
        s != ProcessState::FATAL) {
      all_stopped = false;
      break;
    }
  }

  if (all_stopped) {
    Logs::info() << "[ProcessManager] All processes of " << prog->getName()
                 << " stopped, relaunching...\n";
    prog->setRestarting(false);
    prog->start();
  }
}

void ProcessManager::reap() {
  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    Process *proc = findProcessByPid(pid);
    if (!proc) {
      Logs::warning() << "[ProcessManager] Reaped unknown PID: " << pid << "\n";
      continue;
    }

    int exit_code = 0;
    bool was_signaled = false;

    if (WIFEXITED(status)) {
      exit_code = WEXITSTATUS(status);
      Logs::info() << "[ProcessManager] " << proc->getName() << " (PID " << pid
                   << ") exited with code " << exit_code << "\n";
    } else if (WIFSIGNALED(status)) {
      was_signaled = true;
      Logs::info() << "[ProcessManager] " << proc->getName() << " (PID " << pid
                   << ") killed by signal " << WTERMSIG(status) << "\n";
    }

    if (was_signaled) {
      Program *prog = findProgramByProcess(proc);
      if (prog && prog->isRestarting()) {
        handleRestartingProcess(proc, prog);
      } else {
        proc->setState(ProcessState::STOPPED);
        proc->setEndTime(time(NULL));
        proc->setStatusMsg("Stopped");
        proc->resetRetries();
        proc->resetStopStartTime();
      }
      continue;
    }

    Program *prog = findProgramByProcess(proc);
    if (prog && prog->isRestarting()) {
      handleRestartingProcess(proc, prog);
      continue;
    }

    if (shouldRestart(proc, exit_code)) {
      handleProcessRestart(proc);
    } else {
      proc->setEndTime(time(NULL));
      proc->resetStopStartTime();
      if (isExpectedExitCode(exit_code, proc->getConfig().exitcodes)) {
        proc->setState(ProcessState::EXITED);
        proc->resetRetries();
        Logs::info() << "[ProcessManager] " << proc->getName()
                     << " exited normally\n";
      } else {
        proc->setState(ProcessState::FATAL);
        proc->setStatusMsg("Exited with unexpected code " +
                           std::to_string(exit_code));
        Logs::warning() << "[ProcessManager] " << proc->getName()
                        << " exited with unexpected code " << exit_code << "\n";
      }
    }
  }

  for (auto it = _graveyard.begin(); it != _graveyard.end();) {
    if ((*it)->isFullyStopped()) {
      Logs::debug() << "[ProcessManager] Freeing memory for old program: "
                    << (*it)->getName() << "\n";
      it = _graveyard.erase(it);
    } else {
      ++it;
    }
  }
}

void ProcessManager::updateRunningStates() {
  time_t now = time(NULL);
  for (const auto &[name, prog] : _programs) {
    for (Process *proc : prog->getProcesses()) {
      if (proc->getState() == ProcessState::STARTING) {
        if (now - proc->getStartTime() >= proc->getConfig().starttime) {
          proc->setState(ProcessState::RUNNING);
          proc->resetRetries();
          Logs::debug() << "[ProcessManager] " << proc->getName()
                        << " is now RUNNING (starttime reached)\n";
        }
      } else if (proc->getState() == ProcessState::STOPPING) {
        time_t stop_elapsed = now - proc->getStopStartTime();
        if (stop_elapsed >= proc->getConfig().stoptime) {
          Logs::info() << "[ProcessManager] " << proc->getName()
                       << " did not stop gracefully after "
                       << proc->getConfig().stoptime << "s, sending SIGKILL\n";
          kill(proc->getPid(), SIGKILL);
        }
      }
    }
  }
}
