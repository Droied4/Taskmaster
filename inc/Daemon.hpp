#pragma once

#include "Logs.hpp"
#include "ProcessManager.hpp"
#include "CommandParser.hpp"
#include "Server.hpp"
#include <iostream>
#include <sys/epoll.h>

class Daemon {
private:
  int _epfd;
  int _sig_fd;
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

  void run();
};
