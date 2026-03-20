#pragma once

#include "CommandParser.hpp"
#include "ProcessManager.hpp"
#include "Server.hpp"

class Daemon {
private:
  int _epfd;
  int _sig_fd;
  bool _daemon;
  Server _serv;
  ProcessManager &_manager;
  CommandParser _cparser;

  void setupSignals();

public:
  Daemon(ProcessManager &obj);
  Daemon(const Daemon &obj) = delete;
  Daemon &operator=(const Daemon &obj) = delete;
  ~Daemon();

  static constexpr int EVENTS_SIZE = 64;
  void setDaemon(bool value);

  void run();
};
