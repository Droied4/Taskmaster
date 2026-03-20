#include "Client.hpp"
#include "ResponseFormatter.hpp"
#include "Shell.hpp"

int main() {
  Client client("/tmp/taskmaster.sock");

  ResponseFormatter formatter;

  Shell shell(client, formatter);

  shell.run();
  return 0;
}
