#include "Client.hpp"
#include "ResponseFormatter.hpp"
#include "Shell.hpp"
#include <csignal>

int main() {
  signal(SIGPIPE, SIG_IGN);
  Client client("/tmp/taskmaster.sock");

  ResponseFormatter formatter;

  Shell shell(client, formatter);

  shell.run();
  return 0;
}
