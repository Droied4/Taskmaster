#include "Logs.hpp"
// #include "Server.hpp"

// int main(int ac, char *av[])
// {
//   if (ac >= 1 && av[1])
//   {
// 		 std::string level = av[1];
//
//         if (level == "DEBUG")
//             Logs::setMinLevel(Logs::Level::DEBUG);
//         else if (level == "INFO")
//             Logs::setMinLevel(Logs::Level::INFO);
//         else if (level == "WARNING")
//             Logs::setMinLevel(Logs::Level::WARNING);
//         else if (level == "ERROR")
//             Logs::setMinLevel(Logs::Level::ERROR);
//   }
//   Server serv;
//   serv.run();
//
//   return (0);
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
#ifdef DEBUG
  Logs::setMinLevel(Logs::Level::LDEBUG);
#endif
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
