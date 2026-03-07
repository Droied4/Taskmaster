#include "Server.hpp"

// int main(void) {
//   Server serv;
//
//   serv.run();
//   return (0);
// }

// hasta que tengamos el ProcessManager uwu
#include "ConfigParser.hpp"
#include "Program.hpp"
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

int main() {
  ConfigParser parser;
  auto config_map = parser.parse("config.lua");
  std::vector<Program *> programs;

  for (const auto &[name, cfg] : config_map) {
    Program *prog = new Program(name, cfg);
    programs.push_back(prog);

    if (cfg.autostart) {
      prog->start();
    }
  }

  std::cout << "\n--- waiting for children ---\n";

  // algo asi tendria que hacer el ProcessManager pero mejor xd
  int status;
  pid_t wpid;
  while ((wpid = wait(&status)) > 0) {
    if (WIFEXITED(status)) {
      std::cout << "[Test] PID " << wpid << " finished with exit code "
                << WEXITSTATUS(status) << "\n";
    } else if (WIFSIGNALED(status)) {
      std::cout << "[Test] PID " << wpid << " was killed by signal "
                << WTERMSIG(status) << "\n";
    }
  }

  std::cout << "\n--- finished ---\n";

  for (Program *prog : programs) {
    delete prog;
  }

  return 0;
}
