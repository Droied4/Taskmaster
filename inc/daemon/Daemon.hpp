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
  bool _is_shutting_down;
  ProcessManager _manager;
  CommandParser _cparser;
  Server _serv;
  std::map<int, int> _client_to_pty;
  std::map<int, int> _pty_to_client;

  void setupSignals();
  void handlePTYInput(int client_fd);
  void handlePTYOutput(int pty_fd);
  void handlePTYResize(int pty_fd, std::string &data);
  void handleSignal();
  void processClientCommand(int client_fd, const std::string &input);

public:
  Daemon(struct Config conf);
  Daemon(const Daemon &obj) = delete;
  Daemon &operator=(const Daemon &obj) = delete;
  ~Daemon();

  static constexpr int EVENTS_SIZE = 64;
  void setDaemon(bool value);

  void run();
};
