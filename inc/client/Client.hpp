#pragma once

#include <string>

class Client {
private:
  std::string _socket_path;

public:
  Client(const std::string &socket_path);
  ~Client();

  std::string send(const std::string &command);
  void attachToProcess(int socket_fd);
};
