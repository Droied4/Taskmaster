#include "Daemon.hpp"
#include "Server.hpp"
#include <csignal>
#include <sys/signalfd.h>

Daemon::Daemon() : _epfd(epoll_create1(0)), _serv(_epfd) {}

Daemon::~Daemon() {}

void Daemon::setupSignals() {
  sigset_t mask;      // bitmask for signals
  sigemptyset(&mask); // clear mask
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGHUP);

  if (sigprocmask(SIG_BLOCK, &mask, NULL) ==
      -1) { // poner las señales en el bloque de señales pendientes
    ERROR("sigprocmask failed");
  }

  // SFD_NONBLOCK -> evitar bloqueos,
  // SFD_CLOEXEC -> cerrar este fd en el proceso hijo
  _sig_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);

  // se mete al epoll como si fuera un socket mas
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = _sig_fd;
  epoll_ctl(_epfd, EPOLL_CTL_ADD, _sig_fd, &ev);
}

void Daemon::run() {
  ASSERT(EVENTS_SIZE > 0, "EVENTS_SIZE must be above 0");
  struct epoll_event events[EVENTS_SIZE];
  setupSignals();
  _serv.bindListen();
  while (42) {
    int nfds = epoll_wait(_epfd, events, EVENTS_SIZE, 500);
    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == _serv.getServerFd())
        _serv.acceptConnection(_epfd);
      else if (events[i].data.fd == _sig_fd) { // llega una nueva signal
        struct signalfd_siginfo fdsi;

        ssize_t s = read(_sig_fd, &fdsi, sizeof(struct signalfd_siginfo));

        if (s != sizeof(struct signalfd_siginfo)) {
          if (fdsi.ssi_signo == SIGHUP) {
            Logs::info()
                << "[Daemon] Received SIGHUP, reloading configuration..."
                << std::endl;
            _manager.reloadConfig();
          } else if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
            Logs::info()
                << "[Daemon] Received signal to terminate, shutting down..."
                << std::endl;
            // _manager.shutdownAll();
            return;
          }
        }
      } else {
        _serv.readData(events[i].data.fd,
                       this->_epfd); // agarra el input raw
                                     //
                                     // parser command
                                     //
                                     // _manager.executeCommand(cmd, params);
                                     //
                                     // _serv.sendData
      }
    }
  }
}
