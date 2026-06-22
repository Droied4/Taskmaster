#pragma once
#include <string>
#include <sys/un.h>
class Server {
private:
  int _serv_fd;
  struct sockaddr_un _serv_addr;

  void setServerFd(int fd);

public:
  static constexpr char SOCK_PATH[22] = "/tmp/taskmaster.sock";
  static constexpr int BUFFER_SIZE = 1024;

  Server(int epfd);
  Server(const Server &obj) = delete;
  Server &operator=(const Server &obj) = delete;
  ~Server();

  void acceptConnection(int epfd);
  void bindListen();
  std::string readData(int fd, int epfd);
  void sendData(int client_socket, std::string message,
                bool close_write = true);
  int getServerFd() const;
};
