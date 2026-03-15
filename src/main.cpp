#include "Daemon.hpp"
#include "Logs.hpp"
#include "ProcessManager.hpp"

int main(int argc, char *argv[]) {
#ifdef DEBUG
  Logs::setMinLevel(Logs::Level::LDEBUG);
#endif
  Logs::setMinLevel(Logs::Level::INFO);

  std::string config_path = "config.lua";
  if (argc > 1) {
    config_path = argv[1];
  }

  ProcessManager manager(config_path);

  Daemon daemon(manager);

  daemon.run();

  return 0;
}
