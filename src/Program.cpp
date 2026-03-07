#include "Program.hpp"
#include "common.hpp"
#include <iostream>

Program::Program(const std::string &name, const ProgramConfig &config)
    : _name(name), _config(config) {
  ASSERT(!_name.empty(), "Program name cannot be empty");
  ASSERT(!_config.cmd.empty(), "Program command cannot be empty");
  ASSERT(_config.numprocs > 0, "Program must have at least 1 process");

  // aca tendriamos que pre asignar la memoria para los procesos
}

Program::~Program() {
  // for (Process* p : _processes) {
  //     delete p;
  // }  // o algo asi xd
  _processes.clear();
}

void Program::start() {
  // hasta que tengamos los logs
  std::cout << "[Program] Starting " << _name << " (" << _config.numprocs
            << " processes)...\n";
  // for (Process* p : _processes) {
  //     p->spawn();
  // }
}

void Program::stop() {
  // hasta que tengamos los logs
  std::cout << "[Program] Stopping " << _name << "...\n";
  // for (Process* p : _processes) {
  //     p->killProcess();
  // }
}

void Program::restart() {
  stop();
  start();
}

void Program::monitor() {
  // for (Process* p : _processes) {
  //     p->monitor();
  // }
}

const std::string &Program::getName() const { return _name; }

const ProgramConfig &Program::getConfig() const { return _config; }
