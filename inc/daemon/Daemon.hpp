#pragma once

#include "CommandParser.hpp"
#include "ProcessManager.hpp"
#include "Server.hpp"
#include <map>

struct Config {
  std::string config_path = "config.lua";
  bool daemonize = true;
};

class Daemon {
private:
  int _epfd;
  int _sig_fd;
  bool _daemon;
  Server _serv;
  ProcessManager _manager;
  CommandParser _cparser;
  std::map<int, int> _client_to_pty;
  std::map<int, int> _pty_to_client;

  void setupSignals();

public:
  Daemon(struct Config conf);
  Daemon(const Daemon &obj) = delete;
  Daemon &operator=(const Daemon &obj) = delete;
  ~Daemon();

  static constexpr int EVENTS_SIZE = 64;
  void setDaemon(bool value);

  void run();
};
