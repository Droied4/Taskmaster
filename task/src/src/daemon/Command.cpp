#include "Command.hpp"
#include "Logs.hpp"
#include "Process.hpp"
#include "common.hpp"
#include <algorithm>
#include <csignal>
#include <memory>
#include <string>

static std::vector<Process *>
resolveTarget(std::map<std::string, std::unique_ptr<Program>> &programs,
              const std::string &target, Program **out_prog,
              std::string &error) {
  std::vector<Process *> result;

  if (target == "all") {
    if (out_prog)
      *out_prog = nullptr;

    for (const auto &[prog_name, prog] : programs) {
      for (Process *proc : prog->getProcesses()) {
        ASSERT(proc != nullptr, "Process pointer cannot be null");
        result.push_back(proc);
      }
    }
    return result;
  }

  size_t colon = target.find(':');

  if (colon == std::string::npos) {
    error = "Error: no such process '" + target + "'\n";
    return result;
  }

  std::string prog_name = target.substr(0, colon);
  std::string proc_name = target.substr(colon + 1);

  auto it = programs.find(prog_name);
  if (it == programs.end()) {
    error = "Error: Program group '" + prog_name + "' not found\n";
    return result;
  }

  if (out_prog)
    *out_prog = it->second.get();

  if (proc_name == "*") {
    for (Process *proc : it->second->getProcesses()) {
      ASSERT(proc != nullptr, "Process pointer cannot be null");
      result.push_back(proc);
    }
    return result;
  }

  for (Process *proc : it->second->getProcesses()) {
    ASSERT(proc != nullptr, "Process pointer cannot be null");
    if (proc->getName() == proc_name) {
      result.push_back(proc);
      return result;
    }
  }

  error = "Error: Process '" + proc_name + "' not found in group '" +
          prog_name + "'\n";
  return result;
}

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

static size_t getMaxNameLength(
    const std::map<std::string, std::unique_ptr<Program>> &programs) {
  size_t max_len = 0;
  for (const auto &[prog_name, prog] : programs) {
    for (Process *proc : prog->getProcesses()) {
      std::string display_name = formatDisplayName(proc, prog.get());
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

std::string
Start::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
               const std::vector<std::string> &targets) {
  if (targets.empty())
    return "Error: start command requires a target process or group.\n";

  std::string report = "";
  for (const std::string &target : targets) {
    std::string error;
    Program *prog = nullptr;
    auto procs = resolveTarget(programs, target, &prog, error);

    if (!error.empty()) {
      report += error;
      continue;
    }

    for (Process *proc : procs) {
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
  }
  return report;
}

Stop::Stop() : Command("stop") {}

std::string
Stop::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
              const std::vector<std::string> &targets) {
  if (targets.empty())
    return "Error: stop command requires a target process or group.\n";

  std::string report = "";
  for (const std::string &target : targets) {
    std::string error;
    Program *prog = nullptr;
    auto procs = resolveTarget(programs, target, &prog, error);

    if (!error.empty()) {
      report += error;
      continue;
    }

    for (Process *proc : procs) {
      ASSERT(proc != nullptr, "Program pointer cannot be null");
      proc->killProcess();
      report += proc->getName() + ": stopped\n";
    }
  }
  return report;
}

Status::Status() : Command("status") {}

std::string
Status::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                const std::vector<std::string> &targets) {
  if (programs.empty()) {
    return "No programs configured\n";
  }

  std::string result = "";
  size_t name_width = getMaxNameLength(programs);

  std::vector<std::string> active_targets =
      targets.empty() ? std::vector<std::string>{"all"} : targets;

  for (const std::string &target : active_targets) {
    if (target == "all") {
      for (const auto &[prog_name, prog] : programs) {
        for (Process *proc : prog->getProcesses()) {
          result += formatStatusLine(proc, prog.get(), name_width);
        }
      }
      continue;
    }

    std::string error;
    Program *prog = nullptr;
    auto procs = resolveTarget(programs, target, &prog, error);
    if (!error.empty()) {
      result += error;
      continue;
    }

    for (Process *proc : procs) {
      result += formatStatusLine(proc, prog, name_width);
    }
  }
  return result;
}

Restart::Restart() : Command("restart") {}

std::string
Restart::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                 const std::vector<std::string> &targets) {
  if (targets.empty())
    return "Error: restart command requires a target process or group.\n";

  std::string report = "";
  for (const std::string &target : targets) {
    if (target == "all") {
      for (auto &[name, prog] : programs) {
        prog->restart();
        report += name + ": restarted\n";
      }
      continue;
    }

    Program *prog = nullptr;
    std::string error;
    auto procs = resolveTarget(programs, target, &prog, error);

    if (!error.empty()) {
      report += error;
      continue;
    }

    prog->restartProcesses(procs);
    report += target + ": restarted\n";
  }
  return report;
}

Reload::Reload() : Command("reload") {}

std::string
Reload::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                const std::vector<std::string> &targets) {
  (void)programs;

  if (!targets.empty())
    return "Error: reload command does not take any parameters.\n";

  kill(getpid(), SIGHUP);
  return "taskmasterd: reloading configuration...\n";
}

Pid::Pid() : Command("pid") {}

std::string
Pid::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
             const std::vector<std::string> &targets) {
  (void)programs;
  if (targets.empty())
  	return std::to_string(getpid()) + "\n";

  std::string report = "";
  for (const std::string &target : targets) {
    std::string error;
    Program *prog = nullptr;
    auto procs = resolveTarget(programs, target, &prog, error);

    if (!error.empty()) {
      report += error;
      continue;
    }

    for (Process *proc : procs) {
      ASSERT(proc != nullptr, "Program pointer cannot be null");
	  report += std::to_string(proc->getPid()); 
	  report += "\n"; 
    }
  }
  return report;
}

Shutdown::Shutdown() : Command("shutdown") {}

std::string
Shutdown::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                  const std::vector<std::string> &targets) {
  (void)programs;

  if (!targets.empty())
    return "Error: shutdown command does not take any parameters.\n";

  kill(getpid(), SIGTERM);
  return "taskmasterd: shutting down...\n";
}

Help::Help() : Command("help") {}

std::string
Help::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
              const std::vector<std::string> &targets) {
  (void)programs;

  if (targets.empty()) {
    return "default commands (type help <topic>):\n"
           "=====================================\n"
           "start stop restart status reload pid shutdown help fg\n";
  }

  std::string target = targets[0];

  if (target == "start") {
    return "start <name>         Start a process\n"
           "start <gname:*>      Start all processes in a group\n"
           "start <name> <name>  Start multiple processes\n"
           "start all            Start all processes\n";
  } else if (target == "stop") {
    return "stop <name>          Stop a process\n"
           "stop <gname:*>       Stop all processes in a group\n"
           "stop <name> <name>   Stop multiple processes\n"
           "stop all             Stop all processes\n";
  } else if (target == "restart") {
    return "restart <name>          Restart a process\n"
           "restart <gname:*>       Restart all processes in a group\n"
           "restart <name> <name>   Restart multiple processes\n"
           "restart all             Restart all processes\n";
  } else if (target == "status") {
    return "status <name>          Get status of a process\n"
           "status <gname:*>       Get status of all processes in a group\n"
           "status <name> <name>   Get status of multiple processes\n"
           "status all             Get all processes status info\n";
  } else if (target == "reload") {
    return "reload                 Reload the configuration file\n";
  } else if (target == "pid") {
    return "pid                    Show the PID of the taskmasterd daemon\n";
  } else if (target == "shutdown") {
    return "shutdown               Shutdown the taskmasterd daemon\n";
  } else if (target == "fg") {
    return "fg <gname:name>        Bring a process to the foreground\n";
  }
  return "No help available for '" + target + "'\n";
}

GetPrograms::GetPrograms() : Command("_get_programs") {}

std::string
GetPrograms::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                     const std::vector<std::string> &targets) {
  (void)targets;
  std::string result;
  for (const auto &[name, prog] : programs) {
    result += name + ":*\n";
    for (Process *proc : prog->getProcesses())
      result += name + ":" + proc->getName() + "\n";
  }
  return result;
}

GetCommands::GetCommands() : Command("_get_commands") {}

std::string
GetCommands::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                     const std::vector<std::string> &targets) {
  (void)targets;
  (void)programs;
  return "start\nstop\nstatus\nrestart\nreload\npid\nshutdown\nhelp\nfg\n";
}

Fg::Fg() : Command("fg") {}

std::string
Fg::execute(std::map<std::string, std::unique_ptr<Program>> &programs,
            const std::vector<std::string> &targets) {
  if (targets.empty())
    return "Error: fg requires a target process.\n";

  if (targets.size() > 1)
    return "Error: fg accepts exactly ONE target.\n";

  std::string error;
  Program *prog = nullptr;
  auto procs = resolveTarget(programs, targets[0], &prog, error);

  if (!error.empty())
    return error;

  if (procs.size() != 1)
    return "Error: fg requires exactly one process target\n";

  Process *proc = procs[0];
  if (proc->getState() != ProcessState::RUNNING)
    return "Error: Process is not running.\n";
  if (proc->getPtyMaster() < 0)
    return "Error: Process has no PTY configured.\n";

  return "INTERNAL_ATTACH:" + std::to_string(proc->getPtyMaster()) + ":" +
         proc->getName();
}
