#include "Server.hpp"
#include "Logs.hpp"
#include "common.hpp"
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(int epfd) {
  ASSERT(strlen(SOCK_PATH) > 0, "SOCK_PATH must be declared");
  ASSERT(epfd > 0, "epfd not initialized");
  this->_serv_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (this->_serv_fd < 0)
    ERROR("Sever fd socket failed!");
  Logs::info() << "Server Socket Created\n";
  Logs::debug() << "Epoll Event created with server fd: " << this->_serv_fd
                << "\n";
  bzero(&this->_serv_addr, sizeof(this->_serv_addr));
  this->_serv_addr.sun_family = AF_UNIX;
  strncpy(this->_serv_addr.sun_path, this->SOCK_PATH,
          sizeof(this->_serv_addr.sun_path) - 1);
  this->_serv_addr.sun_path[sizeof(this->_serv_addr.sun_path) - 1] = '\0';

  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLRDHUP;
  ev.data.fd = this->_serv_fd;

  epoll_ctl(epfd, EPOLL_CTL_ADD, this->_serv_fd, &ev);
}

Server::~Server() {
  remove(SOCK_PATH);
  close(this->_serv_fd);
}

void Server::setServerFd(int fd) { this->_serv_fd = fd; }

int Server::getServerFd() const { return (this->_serv_fd); }

void Server::bindListen() {
  if (bind(this->_serv_fd, reinterpret_cast<sockaddr *>(&this->_serv_addr),
           sizeof(this->_serv_addr)) < 0) {
    if (errno == EADDRINUSE) {

      int test = socket(AF_UNIX, SOCK_STREAM, 0);
      if (connect(this->_serv_fd,
                  reinterpret_cast<sockaddr *>(&this->_serv_addr),
                  sizeof(this->_serv_addr)) == 0) {
        close(test);
        std::cerr << "Server already running\n";
        exit(1);
      }
      close(test);
      Logs::debug() << "Unlinked Sock Path: " << SOCK_PATH << "\n";
      unlink(SOCK_PATH);
      if (bind(this->_serv_fd, reinterpret_cast<sockaddr *>(&this->_serv_addr),
               sizeof(this->_serv_addr)) < 0) {
        Logs::error() << "Bind failed\n";
        exit(1);
      }
    }
  }
  Logs::info() << "Server binded on route: " << SOCK_PATH << "\n";
  if (listen(this->_serv_fd, 1) < 0)
    ERROR("Server listen failed!\n");
  Logs::info() << "Server is now Listening!\n";
}

void Server::sendData(int client_socket, std::string message,
                      bool close_write) {
  send(client_socket, message.c_str(), strlen(message.c_str()), 0);
  if (close_write)
    shutdown(client_socket, SHUT_WR);
}

std::string Server::readData(int fd, int epfd) {
  ASSERT(BUFFER_SIZE > 0, "BUFFER_SIZE must be above 0");
  char buffer[BUFFER_SIZE + 1];
  bzero(buffer, BUFFER_SIZE + 1);
  int bytes;

  bytes = read(fd, buffer, BUFFER_SIZE);
  if (bytes <= 0) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    return ("");
  }
  buffer[bytes] = '\0';
  Logs::debug() << "Data read: " << buffer << "\n";
  std::string input(buffer);
  return (input);
}

void Server::acceptConnection(int epfd) {
  int client_socket;

  client_socket = accept(this->_serv_fd, nullptr, nullptr);
  if (client_socket < 0)
    ERROR("Accept connection failed!\n");

  int flags = fcntl(client_socket, F_GETFL, 0);
  fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLRDHUP;
  ev.data.fd = client_socket;
  epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &ev);
}
