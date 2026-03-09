#ifndef SERVER_HPP
#define SERVER_HPP

#include "Logs.hpp"
#include "common.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <vector>

// no he testeado nada pero igual no esta acabado confia tuta

class Server {
private:
  int _epfd;
  int _serv_fd;
  struct sockaddr_un _serv_addr;

  void setServerFd(int fd);

public:
  static constexpr char SOCK_PATH[22] = "/tmp/taskmaster.sock";
  static constexpr int BUFFER_SIZE = 1024;
  static constexpr int EVENTS_SIZE = 64;

  Server();
  Server(const Server &obj);
  Server &operator=(const Server &obj);
  ~Server();

  void bindListen();
  void run();
  void acceptConnection();
  void readData(int fd);
  void sendData();

  int getServerFd() const;
};

#endif
