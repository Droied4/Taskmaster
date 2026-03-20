#include "Program.hpp"
#include "Logs.hpp"
#include "Process.hpp"
#include "common.hpp"
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

Program::Program(const std::string &name, const ProgramConfig &config)
    : _name(name), _config(config), _restarting(false) {
  ASSERT(!_name.empty(), "Program name cannot be empty");
  ASSERT(!_config.cmd.empty(), "Program command cannot be empty");
  ASSERT(_config.numprocs > 0, "Program must have at least 1 process");

  for (int i = 0; i < _config.numprocs; ++i) {
    std::string proc_name;
    char suffix[16];
    snprintf(suffix, sizeof(suffix), "_%02d", i);
    proc_name = _name + suffix;
    _processes.push_back(std::make_unique<Process>(proc_name, _name, _config));
  }
}

Program::~Program() {
  Logs::info() << "[Program] Cleaning up " << _name << "...\n";
  _processes.clear();
}

void Program::start() {
  Logs::info() << "[Program] Starting " << _name << " (" << _config.numprocs
               << " processes)...\n";
  for (auto &proc : _processes) {
    proc->spawn();
  }
}

void Program::stop() {
  Logs::info() << "[Program] Stopping " << _name << "...\n";
  for (auto &proc : _processes) {
    proc->killProcess();
  }
}

void Program::restartProcesses(const std::vector<Process *> &procs) {
  bool any_alive = false;
  for (Process *p : procs) {
    ProcessState state = p->getState();
    if (state == ProcessState::RUNNING || state == ProcessState::STARTING) {
      p->killProcess();
      any_alive = true;
    } else if (state == ProcessState::STOPPING) {
      any_alive = true;
    }
  }
  if (any_alive) {
    setRestarting(true);
  } else {
    start();
  }
}

void Program::restart() {
  Logs::info() << "[Program] Restarting " << _name << "...\n";
  std::vector<Process *> procs;
  for (auto &proc : _processes) {
    procs.push_back(proc.get());
  }
  restartProcesses(procs);
}

bool Program::isFullyStopped() const {
  for (const auto &proc : _processes) {
    ProcessState s = proc->getState();
    if (s == ProcessState::RUNNING || s == ProcessState::STARTING ||
        s == ProcessState::BACKOFF) {
      return false;
    }
  }
  return true;
}

const std::string &Program::getName() const { return _name; }

const ProgramConfig &Program::getConfig() const { return _config; }

std::vector<Process *> Program::getProcesses() const {
  std::vector<Process *> result;
  for (const auto &proc : _processes) {
    result.push_back(proc.get());
  }
  return result;
}

void Program::setRestarting(bool val) { _restarting = val; }

bool Program::isRestarting() const { return _restarting; }
