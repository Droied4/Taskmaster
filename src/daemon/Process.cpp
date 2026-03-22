#include "Process.hpp"
#include "Logs.hpp"
#include "common.hpp"
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

Process::Process(const std::string &name, const std::string &program_name,
                 const ProgramConfig &config)
    : _name(name), _program_name(program_name), _status_msg("Not started"),
      _config(config), _pid(0), _state(ProcessState::STOPPED), _retries(0),
      _pty_master(-1), _start_time(0), _end_time(0), _stop_start_time(0) {
  ASSERT(!_name.empty(), "Process must have a name");
  ASSERT(!_program_name.empty(), "Process must have a program name");
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

int Process::ptySetup() {
  if (_pty_master >= 0) {
    close(_pty_master);
    _pty_master = -1;
  }

  _pty_master = posix_openpt(O_RDWR | O_NOCTTY);
  if (_pty_master < 0) {
    Logs::error() << "Failed to initialize PTY master for: " << _name << "\n";
    return -1;
  }

  if (grantpt(_pty_master) != 0 || unlockpt(_pty_master) != 0) {
    Logs::error() << "Failed to grant/unlock PTY for: " << _name << "\n";
    close(_pty_master);
    _pty_master = -1;
    return -1;
  }

  char *slave_name = ptsname(_pty_master);
  if (!slave_name) {
    Logs::error() << "Failed to get PTY slave name for: " << _name << "\n";
    close(_pty_master);
    _pty_master = -1;
    return -1;
  }

  int pty_slave = open(slave_name, O_RDWR);
  if (pty_slave < 0) {
    Logs::error() << "Failed to open PTY slave for: " << _name << "\n";
    close(_pty_master);
    _pty_master = -1;
    return -1;
  }

  return pty_slave;
}

bool Process::spawn() {
  ASSERT(_state == ProcessState::STOPPED || _state == ProcessState::EXITED ||
             _state == ProcessState::BACKOFF || _state == ProcessState::FATAL,
         "Attempted to spawn a process that is already active");

  std::vector<char *> argv = build_argv();
  std::vector<char *> envp = build_envp();

  ASSERT(argv.size() >= 2, "argv must have at least command and nullptr");
  ASSERT(argv[0] != nullptr, "argv[0] (command) cannot be null");

  int error_pipe[2];
  if (pipe(error_pipe) < 0) {
    Logs::error() << "Failed to create error pipe for: " << _name << "\n";
    for (char *arg : argv)
      if (arg)
        free(arg);
    for (char *env : envp)
      if (env)
        free(env);
    return false;
  }

  fcntl(error_pipe[1], F_SETFD, FD_CLOEXEC);

  int pty_slave = ptySetup();
  if (pty_slave < 0) {
    close(error_pipe[0]);
    close(error_pipe[1]);
    for (char *arg : argv)
      if (arg)
        free(arg);
    for (char *env : envp)
      if (env)
        free(env);
    return false;
  }

  _pid = fork();

  if (_pid < 0) {
    Logs::error() << "Fork failed for process: " << _name << "\n";
    close(error_pipe[0]);
    close(error_pipe[1]);
    _state = ProcessState::FATAL;
    for (char *arg : argv)
      if (arg)
        free(arg);
    for (char *env : envp)
      if (env)
        free(env);
    return false;
  }

  if (_pid == 0) {
    sigset_t empty;
    sigemptyset(&empty);
    sigprocmask(SIG_SETMASK, &empty, NULL);

    close(error_pipe[0]);

    close(_pty_master);
    setsid();

    umask(_config.umask);

    if (!_config.workingdir.empty()) {
      if (chdir(_config.workingdir.c_str()) < 0) {
        int err = errno;
        write(error_pipe[1], &err, sizeof(err));
        _exit(1);
      }
    }

    dup2(pty_slave, STDIN_FILENO);

    if (!_config.stdout_path.empty()) {
      int fd_out = open(_config.stdout_path.c_str(),
                        O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd_out < 0) {
        int err = errno;
        write(error_pipe[1], &err, sizeof(err));
        _exit(1);
      }
      dup2(fd_out, STDOUT_FILENO);
      close(fd_out);
    } else {
      dup2(pty_slave, STDOUT_FILENO);
    }

    if (!_config.stderr_path.empty()) {
      int fd_err = open(_config.stderr_path.c_str(),
                        O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd_err < 0) {
        int err = errno;
        write(error_pipe[1], &err, sizeof(err));
        _exit(1);
      }
      dup2(fd_err, STDERR_FILENO);
      close(fd_err);
    } else {
      dup2(pty_slave, STDERR_FILENO);
    }

    close(pty_slave);
    execvpe(argv[0], argv.data(), envp.data());

    int err = errno;
    write(error_pipe[1], &err, sizeof(err));
    _exit(1);
  }

  close(error_pipe[1]);
  close(pty_slave);

  int flags = fcntl(_pty_master, F_GETFL, 0);
  fcntl(_pty_master, F_SETFL, flags | O_NONBLOCK);

  for (char *arg : argv)
    if (arg)
      free(arg);
  for (char *env : envp)
    if (env)
      free(env);

  int exec_errno = 0;
  ssize_t n = read(error_pipe[0], &exec_errno, sizeof(exec_errno));
  close(error_pipe[0]);

  if (n > 0) {
    std::string cmd_name = _config.cmd;
    size_t space_pos = cmd_name.find(' ');
    if (space_pos != std::string::npos) {
      cmd_name = cmd_name.substr(0, space_pos);
    }
    _status_msg = std::string(strerror(exec_errno)) + ": " + _config.cmd;
    _state = ProcessState::FATAL;
    _end_time = time(NULL);
    Logs::error() << "[Process] " << _name << ": " << _status_msg << "\n";
    return false;
  }

  _state = ProcessState::STARTING;
  _start_time = time(NULL);
  _status_msg = "";

  Logs::debug() << "[Process] Spawning " << _name << " with PID " << _pid
                << "\n";
  return true;
}

void Process::killProcess() {

  Logs::debug() << "[killprocess] " << _name << " state: " << (int)_state
                << "\n";
  if (_pid > 0 &&
      (_state == ProcessState::RUNNING || _state == ProcessState::STARTING)) {
    Logs::debug() << "[Process] Sending signal " << _config.stopsignal << " to "
                  << _name << "\n";
    kill(_pid, _config.stopsignal);
    _state = ProcessState::STOPPING;
    _stop_start_time = time(NULL);
    Logs::debug() << "[Process] " << _name << " killed with signal "
                  << _config.stopsignal << " (stoptime: " << _config.stoptime
                  << "s)\n";
  } else {
    Logs::debug() << "[killprocess] skipped (wrong state or no pid)\n";
  }
}

std::string Process::getUptime() const {
  if (_state != ProcessState::RUNNING && _state != ProcessState::STARTING) {
    return "";
  }

  time_t now = time(NULL);
  time_t elapsed = now - _start_time;

  int days = elapsed / 86400;
  int hours = (elapsed % 86400) / 3600;
  int mins = (elapsed % 3600) / 60;
  int secs = elapsed % 60;

  char buffer[64];
  if (days > 0) {
    snprintf(buffer, sizeof(buffer), "%d days, %d:%02d:%02d", days, hours, mins,
             secs);
  } else {
    snprintf(buffer, sizeof(buffer), "%d:%02d:%02d", hours, mins, secs);
  }
  return std::string(buffer);
}

std::string Process::getFormattedEndTime() const {
  if (_end_time == 0) {
    return "";
  }

  struct tm *tm_info = localtime(&_end_time);
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%b %d %I:%M %p", tm_info);
  return std::string(buffer);
}

void Process::setState(ProcessState state) { _state = state; }

void Process::setEndTime(time_t t) { _end_time = t; }

void Process::setStatusMsg(const std::string &msg) { _status_msg = msg; }

void Process::incrementRetries() { _retries++; }

void Process::resetRetries() { _retries = 0; }

void Process::resetStopStartTime() { _stop_start_time = 0; }

pid_t Process::getPid() const { return _pid; }

ProcessState Process::getState() const { return _state; }

const std::string &Process::getName() const { return _name; }

const std::string &Process::getProgramName() const { return _program_name; }

const ProgramConfig &Process::getConfig() const { return _config; }

time_t Process::getStartTime() const { return _start_time; }

time_t Process::getEndTime() const { return _end_time; }

time_t Process::getStopStartTime() const { return _stop_start_time; }

int Process::getRetries() const { return _retries; }

const std::string &Process::getStatusMsg() const { return _status_msg; }

int Process::getPtyMaster() const { return _pty_master; }
