#include "Server.hpp"
#include "Logs.hpp"

int main(void) {
  Server serv;

  Logs::error() << "test" << std::endl;
  Logs::warning() << std::endl;
  Logs::info() << std::endl;
  Logs::debug() << std::endl;
  serv.run();

  return (0);
}
