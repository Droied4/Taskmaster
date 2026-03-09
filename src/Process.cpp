#include "Process.hpp"
#include "Logs.hpp"
#include "common.hpp"
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

Process::Process(const std::string &name, const ProgramConfig &config)
    : _name(name), _config(config), _pid(0), _state(ProcessState::STOPPED),
      _retries(0), _start_time(0), _end_time(0) {
  ASSERT(!_name.empty(), "Process must have a name");
  ASSERT(!_config.cmd.empty(), "Process must have a command");
}

Process::~Process() {
  if (_state == ProcessState::RUNNING || _state == ProcessState::STARTING) {
    killProcess();
  }
}

std::vector<char *> Process::build_envp() const {
  std::vector<char *> envp;
  for (const auto &[key, val] : _config.env) {
    std::string env_str = key + "=" + val;
    envp.push_back(strdup(env_str.c_str()));
  }
  envp.push_back(nullptr);
  return envp;
}

std::vector<char *> Process::build_argv() const {
  std::vector<char *> argv;
  std::istringstream iss(_config.cmd);
  std::string token;

  while (iss >> token) {
    argv.push_back(strdup(token.c_str()));
  }
  argv.push_back(nullptr);
  return argv;
}

bool Process::spawn() {
  ASSERT(_state == ProcessState::STOPPED || _state == ProcessState::EXITED ||
             _state == ProcessState::BACKOFF,
         "Attempted to spawn a process that is already active");

  std::vector<char *> argv = build_argv();
  std::vector<char *> envp = build_envp();

  _pid = fork();

  if (_pid < 0) {
    Logs::error() << "Fork failed for process: " << _name << "\n";
    for (char *arg : argv)
      if (arg)
        free(arg);
    for (char *env : envp)
      if (env)
        free(env);
    return false;
  }

  if (_pid == 0) {
    umask(_config.umask);

    if (!_config.workingdir.empty()) {
      if (chdir(_config.workingdir.c_str()) < 0) {
        Logs::error() << "Child: Failed to chdir to " << _config.workingdir
                      << "\n";
        exit(1);
      }
    }

    if (!_config.stdout_path.empty()) {
      int fd_out = open(_config.stdout_path.c_str(),
                        O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd_out >= 0) {
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
      }
    }

    if (!_config.stderr_path.empty()) {
      int fd_err = open(_config.stderr_path.c_str(),
                        O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd_err >= 0) {
        dup2(fd_err, STDERR_FILENO);
        close(fd_err);
      }
    }

    execvpe(argv[0], argv.data(), envp.data());

    Logs::error() << "Child: exec failed for " << argv[0] << "\n";
    exit(1);
  }

  for (char *arg : argv)
    if (arg)
      free(arg);
  for (char *env : envp)
    if (env)
      free(env);

  _state = ProcessState::STARTING;
  _start_time = time(NULL);

  Logs::debug() << "[Process] Spawning " << _name << " with PID " << _pid
                << "\n";
  return true;
}

void Process::killProcess() {
  if (_pid > 0 &&
      (_state == ProcessState::RUNNING || _state == ProcessState::STARTING)) {
    Logs::debug() << "[Process] Sending signal " << _config.stopsignal << " to "
                  << _name << "\n";
    kill(_pid, _config.stopsignal);
    Logs::debug() << "[Process] " << _name << " killed with signal "
                  << _config.stopsignal << "\n";
  }
}

pid_t Process::getPid() const { return _pid; }

ProcessState Process::getState() const { return _state; }

const std::string &Process::getName() const { return _name; }
