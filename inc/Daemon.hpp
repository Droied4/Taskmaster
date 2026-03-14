#pragma once

#include "Logs.hpp"
#include "ProcessManager.hpp"
#include "Server.hpp"
#include <iostream>
#include <sys/epoll.h>

class Daemon {
private:
  int _epfd;
  int _sig_fd;
  Server _serv;
  ProcessManager _manager;

  void setupSignals();

public:
  Daemon();
  Daemon(const Daemon &obj) = delete;
  Daemon &operator=(const Daemon &obj) = delete;
  ~Daemon();

  static constexpr int EVENTS_SIZE = 64;

  void run();
};
