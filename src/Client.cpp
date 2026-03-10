#include "Client.hpp"
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

Client::Client(const std::string &socket_path) : _socket_path(socket_path) {}

Client::~Client() {}

std::string Client::send(const std::string &command) {
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    return "Error: Cannot create socket.";
  }

  struct sockaddr_un server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strncpy(server_addr.sun_path, _socket_path.c_str(),
          sizeof(server_addr.sun_path) - 1);

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    close(sock);
    return "Error: taskmasterd is not running or connection refused.";
  }

  if (::send(sock, command.c_str(), command.length(), 0) < 0) {
    close(sock);
    return "Error: Failed to send command to daemon.";
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
