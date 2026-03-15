#pragma once

#include <map>
#include <string>
#include <sys/types.h>
#include <vector>

// solo la declaracion para que no se me rompa todo xd
class Process;

struct ProgramConfig {
  std::string cmd = "";
  int numprocs = 1;
  mode_t umask = 022;
  std::string workingdir = "";
  bool autostart = false;
  std::string autorestart = "unexpected";
  std::vector<int> exitcodes = {0};
  int startretries = 3;
  int starttime = 1;
  int stopsignal = 15;
  int stoptime = 10;
  std::string stdout_path;
  std::string stderr_path;
  std::map<std::string, std::string> env;

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
  ProgramConfig _config;
  std::vector<Process *> _processes;
  bool _restarting;

public:
  Program(const std::string &name, const ProgramConfig &config);
  ~Program();

  Program(const Program &) = delete;
  Program &operator=(const Program &) = delete;

  void start();
  void stop();
  void restart();
  void monitor();

  void setRestarting(bool val);
  bool isRestarting() const;

  const std::string &getName() const;
  const ProgramConfig &getConfig() const;
  const std::vector<Process *> &getProcesses() const;
};
