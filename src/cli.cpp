#include <cstdlib>
#include <cstring>
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>
#include <sstream>
#include <string>
#include <vector>

// hasta que conectemos con el server y ahi vamos a poder obtener la lista real
// de programas y comandos
const std::vector<std::string> commands = {
    "start", "stop", "restart", "status", "reload", "exit", "quit", "help"};
const std::vector<std::string> dummy_programs = {"nginx", "redis", "database",
                                                 "ls", "worker"};

char *command_generator(const char *text, int state) {
  static size_t list_index;
  static int len;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  while (list_index < commands.size()) {
    const std::string &cmd = commands[list_index++];
    if (cmd.compare(0, len, text) == 0) {
      return strdup(cmd.c_str());
    }
  }
  return nullptr;
}

char *program_generator(const char *text, int state) {
  static size_t list_index;
  static int len;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  while (list_index < dummy_programs.size()) {
    const std::string &prog = dummy_programs[list_index++];
    if (prog.compare(0, len, text) == 0) {
      return strdup(prog.c_str());
    }
  }
  return nullptr;
}

char **taskmaster_completion(const char *text, int start, int end) {
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

int main() {
  rl_attempted_completion_function = taskmaster_completion;

  const char *prompt = "\001\033[1;36m\002taskmaster> \001\033[0m\002";

  char *line = nullptr;

  std::cout << "\033[1;35m╔══════════════════════╗\033[0m\n";
  std::cout << "\033[1;35m║\033[0m    Taskmaster CLI \033[1;32m"
               "\033[0m   \033[1;35m║\033[0m\n";
  std::cout << "\033[1;35m╚══════════════════════╝\033[0m\n";
  std::cout << "Use TAB for autocomplete. 'exit' to quit.\n\n";

  while ((line = readline(prompt)) != nullptr) {
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

  clear_history();

  return 0;
}
