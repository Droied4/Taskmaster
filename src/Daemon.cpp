#include "Daemon.hpp"
#include "Server.hpp"
#include <csignal>
#include <sys/signalfd.h>

Daemon::Daemon(ProcessManager &obj)
    : _epfd(epoll_create1(EPOLL_CLOEXEC)), _sig_fd(-1), _serv(_epfd),
      _manager(obj) {
  ASSERT(_epfd >= 0, "Failed to create epoll instance");
  signal(SIGPIPE, SIG_IGN);
}

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
      int client_socket = events[i].data.fd;
      if (client_socket == _serv.getServerFd())
        _serv.acceptConnection(_epfd);
      else if (client_socket == _sig_fd) { // llega una nueva signal
        struct signalfd_siginfo fdsi;

        ssize_t s = read(_sig_fd, &fdsi, sizeof(struct signalfd_siginfo));

        if (s == sizeof(struct signalfd_siginfo)) {
          if (fdsi.ssi_signo == SIGHUP) {
            Logs::info()
                << "[Daemon] Received SIGHUP, reloading configuration..."
                << std::endl;
            _manager.reloadConfig();
          } else if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
            Logs::info()
                << "[Daemon] Received signal to terminate, shutting down..."
                << std::endl;
            _manager.shutdownAll();
            return;
          }
        }
      } else {
        std::string input = _serv.readData(client_socket, this->_epfd);
        if (!input.empty()) {
          _cparser.setCommandParser(input);
          std::string output = _manager.executeCommand(_cparser.getCommand(),
                                                       _cparser.getParams());
          _serv.sendData(client_socket, output);
        }
      }
    }
    _manager.reap();
    _manager.updateRunningStates();
  }
}
