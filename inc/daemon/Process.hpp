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

  std::vector<char *> build_envp() const;
  std::vector<char *> build_argv() const;

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
  int ptySetup();
  void closePty();
  int getPtyMaster() const;
};
