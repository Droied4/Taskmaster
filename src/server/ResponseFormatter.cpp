#include "ResponseFormatter.hpp"
#include <iostream>
#include <sstream>

void ResponseFormatter::print_header() const {
  std::cout << "\033[1;35m╔══════════════════════╗\033[0m\n";
  std::cout << "\033[1;35m║\033[0m   Taskmaster CLI     \033[1;35m║\033[0m\n";
  std::cout << "\033[1;35m╚══════════════════════╝\033[0m\n";
  std::cout
      << "Use TAB for autocomplete. 'help' for commands, 'exit' to quit.\n\n";
}

void ResponseFormatter::print_help() const {
  std::cout << "Available commands:\n"
            << "  start <name>   - Start a process\n"
            << "  stop <name>    - Stop a process\n"
            << "  restart <name> - Restart a process\n"
            << "  status         - Show process status\n"
            << "  reload         - Reload configuration file\n"
            << "  exit/quit      - Close this terminal\n";
}

void ResponseFormatter::print_response(const std::string &raw_response) const {
  if (raw_response.empty()) {
    return;
  }

  if (raw_response.find("Error") == 0) {
    std::cout << "\033[1;31m" << raw_response << "\033[0m";
    return;
  }

  std::istringstream iss(raw_response);
  std::string line;

  while (std::getline(iss, line)) {
    if (line.find("RUNNING") != std::string::npos) {
      std::cout << "\033[1;32m" << line << "\033[0m\n"; // verde
    } else if (line.find("STOPPED") != std::string::npos ||
               line.find("FATAL") != std::string::npos) {
      std::cout << "\033[1;31m" << line << "\033[0m\n"; // rojo
    } else if (line.find("STARTING") != std::string::npos) {
      std::cout << "\033[1;33m" << line << "\033[0m\n"; // amarillo
    } else {
      std::cout << line << "\n";
    }
  }
}
