#include "Program.hpp"
#include "Logs.hpp"
#include "Process.hpp"
#include "common.hpp"

Program::Program(const std::string &name, const ProgramConfig &config)
    : _name(name), _config(config) {
  ASSERT(!_name.empty(), "Program name cannot be empty");
  ASSERT(!_config.cmd.empty(), "Program command cannot be empty");
  ASSERT(_config.numprocs > 0, "Program must have at least 1 process");

  for (int i = 0; i < _config.numprocs; ++i) {
    std::string proc_name = _name + "_" + std::to_string(i);
    _processes.push_back(new Process(proc_name, _config));
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
