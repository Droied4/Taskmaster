#pragma once

#include "Program.hpp"
#include <string>
enum class ProcessState {
  STOPPED,
  STARTING,
  RUNNING,
  STOPPING,
  BACKOFF,
  EXITED,
  FATAL
};

class Process {
private:
  ProgramConfig _config;
  std::string _name;
  std::string _program_name;
  std::string _status_msg;
  int _retries;
  int _pty_master;
  pid_t _pid;
  ProcessState _state;
  time_t _start_time;
  time_t _end_time;
  time_t _stop_start_time;

  std::vector<std::string> buildEnvp() const;
  std::vector<std::string> buildArgv() const;
  void buildExecVectors(std::vector<std::string> &argv_strings,
                        std::vector<std::string> &envp_strings,
                        std::vector<char *> &argv,
                        std::vector<char *> &envp) const;
  bool createErrorPipe(int error_pipe[2]) const;
  bool prepareSpawnResources(int error_pipe[2], int &pty_slave);
  void childWriteErrnoAndExit(int error_pipe_write_fd, int err) const;
  bool setupChildWorkingDirAndUmask(int error_pipe_write_fd) const;
  bool redirectToFileOrPty(const std::string &path, int pty_slave, int target_fd,
                           int error_pipe_write_fd) const;
  bool setupChildFileDescriptors(int pty_slave, int error_pipe_write_fd) const;
  void runChildProcess(int error_pipe[2], int pty_slave, char *const argv[],
                       char *const envp[]);
  bool handleExecFailure(int exec_errno);
  bool finalizeParentSpawn(int error_pipe[2], int pty_slave);

public:
  Process(const std::string &name, const std::string &program_name,
          const ProgramConfig &config);
  ~Process();

  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;

  bool spawn();
  void killProcess();

  pid_t getPid() const;
  ProcessState getState() const;
  const std::string &getName() const;
  const std::string &getProgramName() const;
  const ProgramConfig &getConfig() const;
  time_t getStartTime() const;
  time_t getEndTime() const;
  time_t getStopStartTime() const;
  int getRetries() const;
  const std::string &getStatusMsg() const;
  std::string getUptime() const;
  std::string getFormattedEndTime() const;

  void setStopStartTime(time_t t);
  void setState(ProcessState state);
  void setEndTime(time_t t);
  void setStatusMsg(const std::string &msg);
  void resetStopStartTime();
  void incrementRetries();
  void resetRetries();
  int setupPty();
  void closePty();
  int getPtyMaster() const;
};
