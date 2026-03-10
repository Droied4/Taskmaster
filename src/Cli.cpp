#include "Cli.hpp"

Cli *Cli::instance = nullptr;

Cli::Cli(const std::string &socket_path)
    : _prompt("\001\033[1;36m\002taskmaster> \001\033[0m\002"),
      _socket_path(socket_path) {

  instance = this;

  _commands = {"start",  "stop", "restart", "status",
               "reload", "exit", "quit",    "help"};

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

std::string Cli::send_command(const std::string &cmd) {
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    return "Error: Cannot create socket.\n";
  }

  struct sockaddr_un server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strncpy(server_addr.sun_path, _socket_path.c_str(),
          sizeof(server_addr.sun_path) - 1);

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    close(sock);
    return "Error: taskmasterd is not running or connection refused.\n";
  }

  if (send(sock, cmd.c_str(), cmd.length(), 0) < 0) {
    close(sock);
    return "Error: Failed to send command to daemon.\n";
  }

  char buffer[4096];
  std::string response;
  int bytes_read;

  while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
    buffer[bytes_read] = '\0';
    response += buffer;
  }

  close(sock);
  return response;
}

void Cli::fetch_programs() {
  std::string response = send_command("_get_programs");

  if (response.find("Error:") == 0) {
    std::cerr << "\033[1;33mWarning: taskmasterd is not running. Remote "
                 "commands will fail.\033[0m\n";
    return;
  }

  std::istringstream iss(response);
  std::string prog;
  _programs.clear();
  while (iss >> prog) {
    _programs.push_back(prog);
  }
}
void Cli::run() {
  fetch_programs();

  std::cout << "\033[1;35m╔══════════════════════╗\033[0m\n";
  std::cout << "\033[1;35m║\033[0m   Taskmaster CLI     \033[1;35m║\033[0m\n";
  std::cout << "\033[1;35m╚══════════════════════╝\033[0m\n";
  std::cout
      << "Use TAB for autocomplete. 'help' for commands, 'exit' to quit.\n\n";

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
    } else if (input == "help") {
      std::cout << "Available commands:\n"
                << "  start <name>   - Start a process\n"
                << "  stop <name>    - Stop a process\n"
                << "  restart <name> - Restart a process\n"
                << "  status         - Show process status\n"
                << "  reload         - Reload configuration file\n"
                << "  exit/quit      - Close this terminal\n";
      free(line);
      continue;
    }

    std::string response = send_command(input);

    std::cout << response
              << (response.empty() || response.back() != '\n' ? "\n" : "");

    free(line);
  }

  std::cout << "Terminating CLI connection...\n";
}

int main() {
  Cli taskmaster_cli;
  taskmaster_cli.run();
  return 0;
}
