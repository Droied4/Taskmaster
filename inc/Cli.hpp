#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>
#include <sstream>
#include <string>
#include <vector>

class Cli {
private:
  std::vector<std::string> _commands;
  std::vector<std::string> _programs;
  std::string _prompt;

  static Cli *instance;

  static char *command_generator(const char *text, int state);
  static char *program_generator(const char *text, int state);
  static char **taskmaster_completion(const char *text, int start, int end);

public:
  Cli();
  ~Cli();
  void run();
};
