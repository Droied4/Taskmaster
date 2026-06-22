#pragma once

#include <map>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>

class Process;

struct ProgramConfig {
  mode_t umask = 022;
  bool autostart = false;
  int numprocs = 1;
  int startretries = 3;
  int starttime = 1;
  int stopsignal = 15;
  int stoptime = 10;
  std::string cmd = "";
  std::string workingdir = "";
  std::string autorestart = "unexpected";
  std::string stdout_path;
  std::string stderr_path;
  std::map<std::string, std::string> env;
  std::vector<int> exitcodes = {0};

  bool operator==(const ProgramConfig &other) const {
    return cmd == other.cmd && numprocs == other.numprocs &&
           umask == other.umask && workingdir == other.workingdir &&
           autostart == other.autostart && autorestart == other.autorestart &&
           exitcodes == other.exitcodes && startretries == other.startretries &&
           starttime == other.starttime && stopsignal == other.stopsignal &&
           stoptime == other.stoptime && stdout_path == other.stdout_path &&
           stderr_path == other.stderr_path && env == other.env;
  }

  bool operator!=(const ProgramConfig &other) const {
    return !(*this == other);
  }
};

class Program {
private:
  std::string _name;
  std::vector<std::unique_ptr<Process>> _processes;
  bool _restarting;
  ProgramConfig _config;

public:
  Program(const std::string &name, const ProgramConfig &config);
  ~Program();

  Program(const Program &) = delete;
  Program &operator=(const Program &) = delete;

  void start();
  void stop();
  void restart();
  void restartProcesses(const std::vector<Process *> &procs);

  void setRestarting(bool val);
  bool isRestarting() const;

  const std::string &getName() const;
  const ProgramConfig &getConfig() const;
  std::vector<Process *> getProcesses() const;
  bool isFullyStopped() const;
};
