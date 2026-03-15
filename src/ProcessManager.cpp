#include "ProcessManager.hpp"
#include "common.hpp"
#include <algorithm>
#include <sys/wait.h>

ProcessManager::ProcessManager(const std::string &config_path)
    : _config_path(config_path) {
  ASSERT(!_config_path.empty(), "Config path cannot be empty");

  std::map<std::string, ProgramConfig> configs = _parser.parse(_config_path);

  for (const auto &[name, config] : configs) {
    _programs[name] = new Program(name, config);

    if (config.autostart) {
      _programs[name]->start();
    }
  }
}

ProcessManager::~ProcessManager() {
  for (auto &[name, prog] : _programs) {
    delete prog;
  }
  _programs.clear();
}

Process *ProcessManager::findProcessByPid(pid_t pid) {
  for (const auto &[name, prog] : _programs) {
    for (Process *proc : prog->getProcesses()) {
      if (proc->getPid() == pid) {
        return proc;
      }
    }
  }
  return nullptr;
}

std::string
ProcessManager::executeCommand(const std::string &cmd,
                               const std::vector<std::string> &params) {
  ASSERT(!cmd.empty(), "Command cannot be empty");

  if (cmd == "reload") {
    return _reload_cmd.execute(_programs, "");
  }
  if (cmd == "shutdown") {
    return _shutdown_cmd.execute(_programs, "");
  }
  if (cmd == "status" && params.empty()) {
    return _status_cmd.execute(_programs, "");
  }

  if (params.empty()) {
    return "Error: Command '" + cmd + "' requires at least one target.\n";
  }

  std::string response = "";
  for (const std::string &target : params) {
    if (cmd == "start")
      response += _start_cmd.execute(_programs, target);
    else if (cmd == "stop")
      response += _stop_cmd.execute(_programs, target);
    else if (cmd == "status")
      response += _status_cmd.execute(_programs, target);
    else if (cmd == "restart")
      response += _restart_cmd.execute(_programs, target);
    else
      return "Error: Unknown command '" + cmd + "'.\n";
  }

  return response;
}

void ProcessManager::reloadConfig() {
  Logs::info() << "[ProcessManager] Reloading config from: " << _config_path
               << "\n";

  std::map<std::string, ProgramConfig> new_configs =
      _parser.parse(_config_path);

  for (auto it = _programs.begin(); it != _programs.end();) {
    if (new_configs.find(it->first) == new_configs.end()) {
      it->second->stop();
      delete it->second;
      it = _programs.erase(it);
    } else {
      ++it;
    }
  }

  for (const auto &[name, new_cfg] : new_configs) {
    auto it = _programs.find(name);

    if (it == _programs.end()) {
      _programs[name] = new Program(name, new_cfg);
      if (new_cfg.autostart) {
        _programs[name]->start();
      }
    } else {
      if (it->second->getConfig() != new_cfg) {
        it->second->stop();
        delete it->second;

        _programs[name] = new Program(name, new_cfg);
        _programs[name]->start();
      }
    }
  }
}

void ProcessManager::shutdownAll() {
  for (const auto &[name, prog] : _programs) {
    prog->stop();
  }
}

bool ProcessManager::isExpectedExitCode(int exit_code,
                                        const std::vector<int> &exitcodes) {
  return std::find(exitcodes.begin(), exitcodes.end(), exit_code) !=
         exitcodes.end();
}

bool ProcessManager::shouldRestart(Process *proc, int exit_code) {
  ASSERT(proc != nullptr, "Process cannot be null in shouldRestart");

  const ProgramConfig &config = proc->getConfig();
  const std::string &autorestart = config.autorestart;

  if (autorestart == "never") {
    return false;
  }

  if (autorestart == "always") {
    return true;
  }

  return !isExpectedExitCode(exit_code, config.exitcodes);
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
      int signal_num = WTERMSIG(status);
      Logs::info() << "[ProcessManager] " << proc->getName() << " (PID " << pid
                   << ") killed by signal " << signal_num << "\n";
    }

    if (was_signaled) {
      proc->setState(ProcessState::STOPPED);
      proc->setEndTime(time(NULL));
      proc->setStatusMsg("Stopped");
      proc->resetRetries();
      continue;
    }

    if (shouldRestart(proc, exit_code)) {
      handleProcessRestart(proc);
    } else {
      proc->setEndTime(time(NULL));
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
}

void ProcessManager::updateRunningStates() {
  time_t now = time(NULL);

  for (const auto &[name, prog] : _programs) {
    for (Process *proc : prog->getProcesses()) {
      if (proc->getState() == ProcessState::STARTING) {
        time_t elapsed = now - proc->getStartTime();
        if (elapsed >= proc->getConfig().starttime) {
          proc->setState(ProcessState::RUNNING);
          proc->resetRetries();
          Logs::debug() << "[ProcessManager] " << proc->getName()
                        << " is now RUNNING (starttime reached)\n";
        }
      }
    }
  }
}
