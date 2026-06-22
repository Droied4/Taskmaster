#include "Shell.hpp"
#include <cstring>
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>
#include <sstream>

Shell *Shell::instance = nullptr;

Shell::Shell(Client &client, ResponseFormatter &formatter)
    : _client(client), _formatter(formatter),
      _prompt("\001\033[1;36m\002taskmaster> \001\033[0m\002") {

  instance = this;
  rl_attempted_completion_function = Shell::taskmaster_completion;
}

Shell::~Shell() {
  clear_history();
  instance = nullptr;
}

char *Shell::command_generator(const char *text, int state) {
  static size_t list_index;
  static int len;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  if (!instance)
    return nullptr;

  while (list_index < instance->_commands.size()) {
    const std::string &cmd = instance->_commands[list_index++];
    if (cmd.compare(0, len, text) == 0) {
      return strdup(cmd.c_str());
    }
  }
  return nullptr;
}

char *Shell::program_generator(const char *text, int state) {
  static size_t list_index;
  static int len;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  if (!instance)
    return nullptr;

  while (list_index < instance->_programs.size()) {
    const std::string &prog = instance->_programs[list_index++];
    if (prog.compare(0, len, text) == 0) {
      return strdup(prog.c_str());
    }
  }
  return nullptr;
}

char **Shell::taskmaster_completion(const char *text, int start, int end) {
  (void)end;
  rl_attempted_completion_over = 1;

  if (start == 0) {
    return rl_completion_matches(text, command_generator);
  } else {
    std::string line(rl_line_buffer);
    std::istringstream iss(line);
    std::string first_word;
    iss >> first_word;

    std::string command =
        std::find(instance->_commands.begin(), instance->_commands.end(),
                  first_word) != instance->_commands.end()
            ? first_word
            : "";

    if (!command.empty()) {
      if (command != "help")
        return rl_completion_matches(text, program_generator);
      else
        return rl_completion_matches(text, command_generator);
    }
  }
  return nullptr;
}

void Shell::fetch_programs() {
  std::string response = _client.send("_get_programs");

  if (response.find("Error:") == 0) {
    return;
  }

  std::istringstream iss(response);
  std::string prog;
  _programs.clear();
  while (iss >> prog) {
    _programs.push_back(prog);
  }
}

void Shell::fetch_commands() {
  std::string response = _client.send("_get_commands");

  static bool is_first_call = true;

  if (response.find("Error:") == 0) {
    if (is_first_call)
      std::cerr << "\033[1;33mWarning: taskmasterd is not running.\033[0m\n";
    is_first_call = false;
    return;
  }

  std::istringstream iss(response);
  std::string prog;
  _commands.clear();
  while (iss >> prog) {
    _commands.push_back(prog);
  }
}

void Shell::run() {
  fetch_commands();
  fetch_programs();
  _formatter.print_header();

  char *line = nullptr;

  while ((line = readline(_prompt.c_str())) != nullptr) {
    std::string input(line);

    if (input.empty()) {
      free(line);
      continue;
    }

    add_history(line);

    if (input == "exit" || input == "quit") {
      free(line);
      break;
    } else if (input == "clear") {
      std::cout << "\033[2J\033[H";
      free(line);
      continue;
    }

    std::string raw_response = _client.send(input);
    _formatter.print_response(raw_response);

    free(line);
    fetch_programs();
    fetch_commands();
  }
}
