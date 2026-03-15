#include "Program.hpp"
#include "Logs.hpp"
#include "Process.hpp"
#include "common.hpp"
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

Program::Program(const std::string &name, const ProgramConfig &config)
    : _name(name), _config(config) {
  ASSERT(!_name.empty(), "Program name cannot be empty");
  ASSERT(!_config.cmd.empty(), "Program command cannot be empty");
  ASSERT(_config.numprocs > 0, "Program must have at least 1 process");

  for (int i = 0; i < _config.numprocs; ++i) {
    std::string proc_name;
    if (_config.numprocs == 1) {
      proc_name = _name;
    } else {
      char suffix[16];
      snprintf(suffix, sizeof(suffix), "_%02d", i);
      proc_name = _name + suffix;
    }
    _processes.push_back(new Process(proc_name, _name, _config));
  }
}

Program::~Program() {
  Logs::info() << "[Program] Cleaning up " << _name << "...\n";
  for (Process *p : _processes) {
    delete p;
  }
  _processes.clear();
}

void Program::start() {
  Logs::info() << "[Program] Starting " << _name << " (" << _config.numprocs
               << " processes)...\n";
  for (Process *p : _processes) {
    p->spawn();
  }
}

void Program::stop() {
  Logs::info() << "[Program] Stopping " << _name << "...\n";
  for (Process *p : _processes) {
    p->killProcess();
  }
}

void Program::restart() {
  Logs::info() << "[Program] Restarting " << _name << "...\n";
  stop();

  const int max_wait_ms = 10000;
  const int poll_interval_us = 50000;
  int waited_ms = 0;

  while (waited_ms < max_wait_ms) {
    bool all_stopped = true;
    for (Process *p : _processes) {
      ProcessState state = p->getState();
      if (state != ProcessState::STOPPED && state != ProcessState::EXITED &&
          state != ProcessState::FATAL) {
        all_stopped = false;
        break;
      }
    }

    if (all_stopped) {
      break;
    }

    int status;
    pid_t reaped;
    while ((reaped = waitpid(-1, &status, WNOHANG)) > 0) {
      for (Process *p : _processes) {
        if (p->getPid() == reaped) {
          p->setState(ProcessState::STOPPED);
          p->setEndTime(time(NULL));
          p->setStatusMsg("Stopped");
          break;
        }
      }
    }

    usleep(poll_interval_us);
    waited_ms += poll_interval_us / 1000;
  }

  start();
}

void Program::monitor() {
  // for (Process *p : _processes) {
  //   p->monitor();
  // }
}

const std::string &Program::getName() const { return _name; }

const ProgramConfig &Program::getConfig() const { return _config; }

const std::vector<Process *> &Program::getProcesses() const {
  return _processes;
}
