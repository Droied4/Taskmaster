#include "Command.hpp"
#include "Process.hpp"
#include <csignal>

static std::string stateToString(ProcessState state) {

  switch (state) {
  case ProcessState::STOPPED:
    return "STOPPED";
  case ProcessState::STARTING:
    return "STARTING";
  case ProcessState::RUNNING:
    return "RUNNING";
  case ProcessState::BACKOFF:
    return "BACKOFF";
  case ProcessState::EXITED:
    return "EXITED";
  case ProcessState::FATAL:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}

Command::Command(std::string name) : _name(name) {}

Command::~Command() {}

Start::Start() : Command("start") {}

std::string Start::execute(std::map<std::string, Program *> &programs,
                           const std::string &target) {
  auto it = programs.find(target);

  if (it != programs.end()) {
    it->second->start();
    return target + ": started\n";
  }
  return target + ": ERROR (no such process)\n";
}

Stop::Stop() : Command("stop") {}

std::string Stop::execute(std::map<std::string, Program *> &programs,
                          const std::string &target) {
  auto it = programs.find(target);
  if (it != programs.end()) {
    it->second->stop();
    return target + ": stopped\n";
  }
  return target + ": ERROR (no such process)\n";
}

Status::Status() : Command("status") {}

std::string Status::execute(std::map<std::string, Program *> &programs,
                            const std::string &target) {

  std::string result = "";

  if (target.empty() || target == "all") {
    if (programs.empty())
      return "No programs configured\n";

    for (const auto &[prog_name, prog] : programs) {
      for (Process *proc : prog->getProcesses()) {
        result +=
            proc->getName() + " | " + stateToString(proc->getState()) +
            " | PID: " +
            (proc->getPid() > 0 ? std::to_string(proc->getPid()) : "N/A") +
            "\n";
      }
    }
    return result;
  }

  auto it = programs.find(target);

  if (it != programs.end()) {
    for (Process *proc : it->second->getProcesses()) {
      result += proc->getName() + " | " + stateToString(proc->getState()) +
                " | PID: " +
                (proc->getPid() > 0 ? std::to_string(proc->getPid()) : "N/A") +
                "\n";
    }
    return result;
  }

  return "Error: Program `" + target + "` not found\n";
}

Restart::Restart() : Command("restart") {}

std::string Restart::execute(std::map<std::string, Program *> &programs,
                             const std::string &target) {
  auto it = programs.find(target);
  if (it != programs.end()) {
    it->second->restart();
    return target + ": restarted\n";
  }
  return "" + target + ": ERROR (no such process)\n";
}

Reload::Reload() : Command("reload") {}

std::string Reload::execute(std::map<std::string, Program *> &programs,
                            const std::string &target) {
  (void)programs;
  (void)target;

  kill(getpid(), SIGHUP);
  return "taskmasterd: reloading configuration...\n";
}

Shutdown::Shutdown() : Command("shutdown") {}

std::string Shutdown::execute(std::map<std::string, Program *> &programs,
                              const std::string &target) {
  (void)programs;
  (void)target;

  kill(getpid(), SIGTERM);
  return "taskmasterd: shutting down...\n";
}
