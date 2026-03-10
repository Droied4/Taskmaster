#pragma once

#include "Client.hpp"
#include "ResponseFormatter.hpp"
#include <readline/history.h>
#include <readline/readline.h>
#include <string>
#include <vector>

class Shell {
private:
  Client &_client;
  ResponseFormatter &_formatter;

  std::string _prompt;
  std::vector<std::string> _commands;
  std::vector<std::string> _programs;

  static Shell *instance;

  static char *command_generator(const char *text, int state);
  static char *program_generator(const char *text, int state);
  static char **taskmaster_completion(const char *text, int start, int end);

  void fetch_programs();

public:
  Shell(Client &client, ResponseFormatter &formatter);
  ~Shell();
  void run();
};
