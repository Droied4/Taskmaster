#include "Daemon.hpp"
#include "Server.hpp"

Daemon::Daemon() : _epfd(epoll_create1(0)), _serv(_epfd)
{}

Daemon::~Daemon()
{}

void Daemon::run() {
ASSERT(EVENTS_SIZE > 0, "EVENTS_SIZE must be above 0");
  struct epoll_event events[EVENTS_SIZE];
  _serv.bindListen();
  while (42) {
    int nfds = epoll_wait(_epfd, events, EVENTS_SIZE, 500);
    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == _serv.getServerFd())
        _serv.acceptConnection(_epfd);
      else
        _serv.readData(events[i].data.fd, this->_epfd);
    }
  }
}
