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
#include <vector>

int main() {
  ConfigParser parser;

  std::map<std::string, ProgramConfig> config_map = parser.parse("config.lua");

  std::vector<Program *> programs;

  for (const auto &pair : config_map) {
    Program *prog = new Program(pair.first, pair.second);
    programs.push_back(prog);

    if (prog->getConfig().autostart) {
      prog->start();
    }
  }

  for (Program *prog : programs) {
    delete prog;
  }

  return 0;
}
