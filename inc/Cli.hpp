#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>
#include <sstream>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

class Cli {
private:
  std::vector<std::string> _commands;
  std::vector<std::string> _programs;
  std::string _prompt;
  std::string _socket_path;

  static Cli *instance;

  static char *command_generator(const char *text, int state);
  static char *program_generator(const char *text, int state);
  static char **taskmaster_completion(const char *text, int start, int end);

  std::string send_command(const std::string &cmd);
  void fetch_programs();

public:
  Cli(const std::string &socket_path = "/tmp/taskmaster.sock");
  ~Cli();
  void run();
};
