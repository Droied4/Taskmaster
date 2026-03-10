#include "Cli.hpp"

Cli *Cli::instance = nullptr;

Cli::Cli() : _prompt("\001\033[1;36m\002taskmaster> \001\033[0m\002") {
  instance = this;

  _commands = {"start",  "stop", "restart", "status",
               "reload", "exit", "quit",    "help"};
  _programs = {"nginx", "redis", "database", "ls", "worker"};

  rl_attempted_completion_function = Cli::taskmaster_completion;
}

Cli::~Cli() {
  clear_history();
  instance = nullptr;
}

char *Cli::command_generator(const char *text, int state) {
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

char *Cli::program_generator(const char *text, int state) {
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

char **Cli::taskmaster_completion(const char *text, int start, int end) {
  (void)end;
  rl_attempted_completion_over = 1;

  if (start == 0) {
    return rl_completion_matches(text, command_generator);
  } else {
    std::string line(rl_line_buffer);
    std::istringstream iss(line);
    std::string first_word;
    iss >> first_word;

    if (first_word == "start" || first_word == "stop" ||
        first_word == "restart") {
      return rl_completion_matches(text, program_generator);
    }
  }
  return nullptr;
}

void Cli::run() {
  std::cout << "\033[1;35m╔══════════════════════╗\033[0m\n";
  std::cout << "\033[1;35m║\033[0m    Taskmaster CLI \033[1;32m\033[0m   "
               "\033[1;35m║\033[0m\n";
  std::cout << "\033[1;35m╚══════════════════════╝\033[0m\n";
  std::cout << "Use TAB for autocomplete. 'exit' to quit.\n\n";

  char *line = nullptr;

  while ((line = readline(_prompt.c_str())) != nullptr) {
    std::string input(line);

    if (input.empty()) {
      free(line);
      continue;
    }

    if (input == "exit" || input == "quit") {
      free(line);
      break;
    }

    add_history(line);
    std::cout << "Command registered for server: " << input << "\n";

    free(line);
  }

  std::cout << "\nTerminating CLI connection...\n";
}

int main() {
  Cli taskmaster_cli;
  taskmaster_cli.run();
  return 0;
}
