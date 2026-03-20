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

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 150000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  struct sockaddr_un server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;

  size_t len =
      std::min(_socket_path.length(), sizeof(server_addr.sun_path) - 1);
  std::copy(_socket_path.begin(), _socket_path.begin() + len,
            server_addr.sun_path);
  server_addr.sun_path[len] = '\0';

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    close(sock);
    return "Error: taskmasterd is not running or connection refused.\n";
  }

  if (::send(sock, command.c_str(), command.length(), 0) < 0) {
    close(sock);
    return "Error: Failed to send command to daemon.";
  }

  std::string response;
  char buffer[4096];
  int bytes_read;
  while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
    response.append(buffer, bytes_read);
  }
  close(sock);
  return response;
}
