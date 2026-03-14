#include "ProcessManager.hpp"

ProcessManager::ProcessManager(
    const std::map<std::string, ProgramConfig> &configs) {
  for (const auto &[name, config] : configs) {
    _programs[name] = new Program(name, config);
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
  std::string response = "";

  if (params.empty()) {
    if (cmd == "status") {
      return _status_cmd.execute(_programs, "");
    } else if (cmd == "reload" || cmd == "shutdown") {
    } else {
      return "Error: Command '" + cmd + "' requires at least one target.\n";
    }
  }

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
  std::map<std::string, ProgramConfig> new_configs =
      _parser.parse("config.lua");

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
