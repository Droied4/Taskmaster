#pragma once
#include "Program.hpp"
#include <ctime>
#include <string>
#include <sys/types.h>
#include <vector>

enum class ProcessState { STOPPED, STARTING, RUNNING, BACKOFF, EXITED, FATAL };

class Process {
private:
  std::string _name;
  ProgramConfig _config;
  pid_t _pid;
  ProcessState _state;
  int _retries;
  time_t _start_time;
  time_t _end_time;

  std::vector<char *> build_envp() const;
  std::vector<char *> build_argv() const;

public:
  Process(const std::string &name, const ProgramConfig &config);
  ~Process();

  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;

  bool spawn();
  void killProcess();

  pid_t getPid() const;
  ProcessState getState() const;
  const std::string &getName() const;
};
