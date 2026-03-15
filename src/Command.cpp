#include "Command.hpp"
#include "Process.hpp"
#include "common.hpp"
#include <algorithm>
#include <csignal>

static std::string stateToString(ProcessState state) {
  switch (state) {
  case ProcessState::STOPPED:
    return "STOPPED";
  case ProcessState::STARTING:
    return "STARTING";
  case ProcessState::RUNNING:
    return "RUNNING";
  case ProcessState::STOPPING:
    return "STOPPING";
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

static std::string formatDisplayName(Process *proc, Program *prog) {
  if (prog->getConfig().numprocs == 1) {
    return proc->getName();
  }
  return prog->getName() + ":" + proc->getName();
}

static std::string formatStateInfo(Process *proc) {
  ProcessState state = proc->getState();

  switch (state) {
  case ProcessState::RUNNING:
  case ProcessState::STARTING:
    return "pid " + std::to_string(proc->getPid()) + ", uptime " +
           proc->getUptime();
  case ProcessState::STOPPING:
    return "pid " + std::to_string(proc->getPid()) + ", stopping";
  case ProcessState::EXITED:
    return proc->getFormattedEndTime();
  case ProcessState::FATAL:
    return proc->getStatusMsg();
  case ProcessState::STOPPED:
    return proc->getStatusMsg().empty() ? "Not started" : proc->getStatusMsg();
  case ProcessState::BACKOFF:
    return proc->getStatusMsg().empty() ? "Restarting..."
                                        : proc->getStatusMsg();
  default:
    return "";
  }
}

static size_t
getMaxNameLength(const std::map<std::string, Program *> &programs) {
  size_t max_len = 0;
  for (const auto &[prog_name, prog] : programs) {
    for (Process *proc : prog->getProcesses()) {
      std::string display_name = formatDisplayName(proc, prog);
      max_len = std::max(max_len, display_name.length());
    }
  }
  return max_len;
}

static std::string formatStatusLine(Process *proc, Program *prog,
                                    size_t name_width) {
  std::string display_name = formatDisplayName(proc, prog);
  std::string state_str = stateToString(proc->getState());
  std::string info = formatStateInfo(proc);

  std::string padded_name = display_name;
  if (padded_name.length() < name_width) {
    padded_name.append(name_width - padded_name.length(), ' ');
  }

  std::string padded_state = state_str;
  if (padded_state.length() < 10) {
    padded_state.append(10 - padded_state.length(), ' ');
  }

  return padded_name + " " + padded_state + " " + info + "\n";
}

Command::Command(std::string name) : _name(name) {}

Command::~Command() {}

Start::Start() : Command("start") {}

std::string Start::execute(std::map<std::string, Program *> &programs,
                           const std::string &target) {
  ASSERT(!target.empty(), "Start command requires a target");

  auto it = programs.find(target);
  if (it == programs.end())
    return "Error: Program '" + target + "' not found\n";

  ASSERT(it->second != nullptr, "Program pointer cannot be null");

  std::string report = "";
  for (Process *proc : it->second->getProcesses()) {
    ASSERT(proc != nullptr, "Process pointer cannot be null");
    ProcessState s = proc->getState();

    if (s == ProcessState::STOPPED || s == ProcessState::EXITED ||
        s == ProcessState::FATAL) {
      proc->spawn();
      report += proc->getName() + ": started\n";
    } else {
      report += proc->getName() +
                ": already active (State: " + stateToString(s) + ")\n";
    }
  }
  return report;
}

Stop::Stop() : Command("stop") {}

std::string Stop::execute(std::map<std::string, Program *> &programs,
                          const std::string &target) {
  ASSERT(!target.empty(), "Stop command requires a target");

  auto it = programs.find(target);
  if (it != programs.end()) {
    ASSERT(it->second != nullptr, "Program pointer cannot be null");
    it->second->stop();
    return target + ": stopped\n";
  }
  return target + ": ERROR (no such process)\n";
}

Status::Status() : Command("status") {}

std::string Status::execute(std::map<std::string, Program *> &programs,
                            const std::string &target) {
  if (programs.empty()) {
    return "No programs configured\n";
  }

  std::string result = "";
  size_t name_width = getMaxNameLength(programs);

  if (target.empty() || target == "all") {
    for (const auto &[prog_name, prog] : programs) {
      for (Process *proc : prog->getProcesses()) {
        result += formatStatusLine(proc, prog, name_width);
      }
    }
    return result;
  }

  auto it = programs.find(target);
  if (it != programs.end()) {
    for (Process *proc : it->second->getProcesses()) {
      result += formatStatusLine(proc, it->second, name_width);
    }
    return result;
  }

  return "Error: Program '" + target + "' not found\n";
}

Restart::Restart() : Command("restart") {}

std::string Restart::execute(std::map<std::string, Program *> &programs,
                             const std::string &target) {
  ASSERT(!target.empty(), "Restart command requires a target");

  auto it = programs.find(target);
  if (it != programs.end()) {
    ASSERT(it->second != nullptr, "Program pointer cannot be null");
    it->second->setRestarting(true);
    it->second->restart();
    return target + ": restarting\n";
  }
  return target + ": ERROR (no such process)\n";
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
