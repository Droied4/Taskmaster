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
    : _config(config), _name(name), _program_name(program_name),
      _status_msg("Not started"), _retries(0), _pty_master(-1), _pid(0),
      _state(ProcessState::STOPPED), _start_time(0), _end_time(0),
      _stop_start_time(0) {
  ASSERT(!_name.empty(), "Process must have a name");
  ASSERT(!_program_name.empty(), "Process must have a program name");
  ASSERT(!_config.cmd.empty(), "Process must have a command");
}

Process::~Process() {
  if (_state == ProcessState::RUNNING || _state == ProcessState::STARTING) {
    killProcess();
  }
  closePty();
}

std::vector<std::string> Process::buildEnvp() const {
  std::vector<std::string> env_strings;
  for (const auto &[key, val] : _config.env) {
    env_strings.push_back(key + "=" + val);
  }
  return env_strings;
}

std::vector<std::string> Process::buildArgv() const {
  std::vector<std::string> argv_strings;
  std::istringstream iss(_config.cmd);
  std::string token;
  while (iss >> token) {
    argv_strings.push_back(token);
  }
  return argv_strings;
}

int Process::setupPty() {
  if (_pty_master >= 0) {
    close(_pty_master);
    _pty_master = -1;
  }

  _pty_master = posix_openpt(O_RDWR | O_NOCTTY);
  if (_pty_master < 0) {
    Logs::error() << "Failed to initialize PTY master for: " << _name << std::endl;
    return -1;
  }

  if (grantpt(_pty_master) != 0 || unlockpt(_pty_master) != 0) {
    Logs::error() << "Failed to grant/unlock PTY for: " << _name << std::endl;
    close(_pty_master);
    _pty_master = -1;
    return -1;
  }

  char *slave_name = ptsname(_pty_master);
  if (!slave_name) {
    Logs::error() << "Failed to get PTY slave name for: " << _name << std::endl;
    close(_pty_master);
    _pty_master = -1;
    return -1;
  }

  int pty_slave = open(slave_name, O_RDWR);
  if (pty_slave < 0) {
    Logs::error() << "Failed to open PTY slave for: " << _name << std::endl;
    close(_pty_master);
    _pty_master = -1;
    return -1;
  }

  return pty_slave;
}

void Process::buildExecVectors(std::vector<std::string> &argv_strings,
                               std::vector<std::string> &envp_strings,
                               std::vector<char *> &argv,
                               std::vector<char *> &envp) const {
  argv_strings = buildArgv();
  envp_strings = buildEnvp();

  argv.clear();
  for (std::string &str : argv_strings) {
    argv.push_back(str.data());
  }
  argv.push_back(nullptr);

  envp.clear();
  for (std::string &str : envp_strings) {
    envp.push_back(str.data());
  }
  envp.push_back(nullptr);

  ASSERT(argv.size() >= 2, "argv must have at least command and nullptr");
  ASSERT(argv[0] != nullptr, "argv[0] (command) cannot be null");
}

bool Process::createErrorPipe(int error_pipe[2]) const {
  if (pipe(error_pipe) < 0) {
    Logs::error() << "Failed to create error pipe for: " << _name << std::endl;
    return false;
  }
  fcntl(error_pipe[1], F_SETFD, FD_CLOEXEC);
  return true;
}

bool Process::prepareSpawnResources(int error_pipe[2], int &pty_slave) {
  if (!createErrorPipe(error_pipe)) {
    return false;
  }

  pty_slave = setupPty();
  if (pty_slave < 0) {
    close(error_pipe[0]);
    close(error_pipe[1]);
    return false;
  }

  return true;
}

void Process::childWriteErrnoAndExit(int error_pipe_write_fd, int err) const {
  write(error_pipe_write_fd, &err, sizeof(err));
  _exit(1);
}

bool Process::setupChildWorkingDirAndUmask(int error_pipe_write_fd) const {
  umask(_config.umask);

  if (_config.workingdir.empty()) {
    return true;
  }

  if (chdir(_config.workingdir.c_str()) < 0) {
    childWriteErrnoAndExit(error_pipe_write_fd, errno);
    return false;
  }

  return true;
}

bool Process::redirectToFileOrPty(const std::string &path, int pty_slave,
                                  int target_fd, int error_pipe_write_fd) const {
  if (path.empty()) {
    dup2(pty_slave, target_fd);
    return true;
  }

  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd < 0) {
    childWriteErrnoAndExit(error_pipe_write_fd, errno);
    return false;
  }

  dup2(fd, target_fd);
  close(fd);
  return true;
}

bool Process::setupChildFileDescriptors(int pty_slave, int error_pipe_write_fd) const {
  dup2(pty_slave, STDIN_FILENO);

  if (!redirectToFileOrPty(_config.stdout_path, pty_slave, STDOUT_FILENO,
                           error_pipe_write_fd)) {
    return false;
  }

  if (!redirectToFileOrPty(_config.stderr_path, pty_slave, STDERR_FILENO,
                           error_pipe_write_fd)) {
    return false;
  }

  return true;
}

void Process::runChildProcess(int error_pipe[2], int pty_slave, char *const argv[],
                              char *const envp[]) {
  sigset_t empty;
  sigemptyset(&empty);
  sigprocmask(SIG_SETMASK, &empty, NULL);

  close(error_pipe[0]);
  close(_pty_master);
  setsid();

  if (!setupChildWorkingDirAndUmask(error_pipe[1])) {
    _exit(1);
  }

  if (!setupChildFileDescriptors(pty_slave, error_pipe[1])) {
    _exit(1);
  }

  close(pty_slave);
  execvpe(argv[0], argv, envp);
  childWriteErrnoAndExit(error_pipe[1], errno);
}

bool Process::handleExecFailure(int exec_errno) {
  _status_msg = std::string(strerror(exec_errno)) + ": " + _config.cmd;
  _state = ProcessState::FATAL;
  _end_time = time(NULL);
  Logs::error() << "[Process] " << _name << ": " << _status_msg << std::endl;
  return false;
}

bool Process::finalizeParentSpawn(int error_pipe[2], int pty_slave) {
  close(error_pipe[1]);
  close(pty_slave);

  int flags = fcntl(_pty_master, F_GETFL, 0);
  fcntl(_pty_master, F_SETFL, flags | O_NONBLOCK);

  int exec_errno = 0;
  ssize_t n = read(error_pipe[0], &exec_errno, sizeof(exec_errno));
  close(error_pipe[0]);

  if (n > 0) {
    return handleExecFailure(exec_errno);
  }

  return true;
}

bool Process::spawn() {
  ASSERT(_state == ProcessState::STOPPED || _state == ProcessState::EXITED ||
             _state == ProcessState::BACKOFF || _state == ProcessState::FATAL,
         "Attempted to spawn a process that is already active");

  std::vector<std::string> argv_strings;
  std::vector<std::string> envp_strings;
  std::vector<char *> argv;
  std::vector<char *> envp;
  buildExecVectors(argv_strings, envp_strings, argv, envp);

  int error_pipe[2];
  int pty_slave = -1;
  if (!prepareSpawnResources(error_pipe, pty_slave)) {
    return false;
  }

  _pid = fork();

  if (_pid < 0) {
    Logs::error() << "Fork failed for process: " << _name << std::endl;
    close(error_pipe[0]);
    close(error_pipe[1]);
    close(pty_slave);
    _state = ProcessState::FATAL;
    return false;
  }

  if (_pid == 0) {
    runChildProcess(error_pipe, pty_slave, argv.data(), envp.data());
  }

  if (!finalizeParentSpawn(error_pipe, pty_slave)) {
    return false;
  }

  _state = ProcessState::STARTING;
  _start_time = time(NULL);
  _status_msg = "";

  Logs::debug() << "[Process] Spawning " << _name << " with PID " << _pid
                << std::endl;
  return true;
}

void Process::killProcess() {

  Logs::debug() << "[killprocess] " << _name << " state: " << (int)_state
                << std::endl;
  if (_pid > 0 &&
      (_state == ProcessState::RUNNING || _state == ProcessState::STARTING)) {
    Logs::debug() << "[Process] Sending signal " << _config.stopsignal << " to "
                  << _name << std::endl;
    kill(_pid, _config.stopsignal);
    _state = ProcessState::STOPPING;
    _stop_start_time = time(NULL);
    Logs::debug() << "[Process] " << _name << " killed with signal "
                  << _config.stopsignal << " (stoptime: " << _config.stoptime
                  << "s)" << std::endl;
  } else {
    Logs::debug() << "[killprocess] skipped (wrong state or no pid)" << std::endl;
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

void Process::closePty() {
  if (_pty_master >= 0) {
    close(_pty_master);
    _pty_master = -1;
  }
}

void Process::setStopStartTime(time_t t) { _stop_start_time = t; }

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
