#pragma once
#include <string>
#include <sys/un.h>
class Server {
private:
  int _epfd;
  int _serv_fd;
  struct sockaddr_un _serv_addr;

  void setServerFd(int fd);

public:
  static constexpr char SOCK_PATH[22] = "/tmp/taskmaster.sock";
  static constexpr int BUFFER_SIZE = 1024;

  Server(int epfd);
  Server(const Server &obj);
  Server &operator=(const Server &obj);
  ~Server();

  void acceptConnection(int epfd);
  void bindListen();
  std::string readData(int fd, int epfd);
  void sendData(int client_socket, std::string message);
  int getServerFd() const;
};
