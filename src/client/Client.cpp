#include "Client.hpp"
#include <cstring>
#include <iostream>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <termios.h>
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

  int bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);

  if (bytes_read > 0) {
    response.append(buffer, bytes_read);

    if (response.find("ATTACH_OK") != std::string::npos) {
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

      attachToProcess(sock);

      close(sock);
      return "";
    }

    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
      response.append(buffer, bytes_read);
    }
  }

  close(sock);
  return response;
}
void Client::attachToProcess(int socket_fd) {
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);

  newt = oldt;

  newt.c_lflag &= ~(ICANON | ECHO | ISIG);

  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  struct pollfd fds[2];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = socket_fd;
  fds[1].events = POLLIN;

  char buf[1024];

  bool attached = true;

  std::cout << "\r\n[Attached to process. Press Ctrl+Q to detach]\r\n";

  while (attached) {
    if (poll(fds, 2, -1) < 0)
      break;

    if (fds[0].revents & POLLIN) {
      int n = read(STDIN_FILENO, buf, sizeof(buf));
      if (n > 0) {
        for (int i = 0; i < n; ++i) {
          if (buf[i] == 3) { // Ctrl+C
            attached = false;
            break;
          }
        }
        if (attached) {
          write(socket_fd, buf, n);
        }
      }
    }

    if (fds[1].revents & POLLIN) {
      int n = read(socket_fd, buf, sizeof(buf));
      if (n <= 0) {
        attached = false;
      } else {
        write(STDOUT_FILENO, buf, n);
      }
    }
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  std::cout << "\r\n[Detached from process]\r\n";
}
