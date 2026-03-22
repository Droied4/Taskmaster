#include "Daemon.hpp"
#include "Logs.hpp"
#include "Process.hpp"
#include "Server.hpp"
#include "common.hpp"
#include <csignal>
#include <ostream>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <unistd.h>

Daemon::Daemon(struct Config conf)
    : _epfd(epoll_create1(EPOLL_CLOEXEC)), _sig_fd(-1), _daemon(conf.daemonize),
      _serv(_epfd), _manager(conf.config_path) {
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
  // sigaddset(&mask, SIGCHLD);

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

void Daemon::setDaemon(bool value) { this->_daemon = value; }

void Daemon::run() {
  ASSERT(EVENTS_SIZE > 0, "EVENTS_SIZE must be above 0");
  if (_daemon) {
    if (daemon(1, 0) == -1)
      ERROR("daemon failed");
  }
  struct epoll_event events[EVENTS_SIZE];
  setupSignals();
  _serv.bindListen();
  signal(SIGPIPE, SIG_IGN);
  Logs::debug() << "pid: " << getpid() << "\n";
  while (42) {
    int nfds = epoll_wait(_epfd, events, EVENTS_SIZE, 100);
    for (int i = 0; i < nfds; ++i) {
      int current_fd = events[i].data.fd;
      if (current_fd == _serv.getServerFd())
        _serv.acceptConnection(_epfd);
      else if (current_fd == _sig_fd) { // llega una nueva signal
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
      } else if (_client_to_pty.find(current_fd) != _client_to_pty.end()) {
        char buf[1024];
        int bytes = read(current_fd, buf, sizeof(buf));
        int pty_fd = _client_to_pty[current_fd];

        if (bytes <= 0) {
          Logs::info() << "[Daemon] Client detached from PTY\n";
          epoll_ctl(_epfd, EPOLL_CTL_DEL, current_fd, nullptr);
          _client_to_pty.erase(current_fd);
          _pty_to_client.erase(pty_fd);
          close(current_fd);
        } else {
          write(pty_fd, buf, bytes);
        }
      } else if (_pty_to_client.find(current_fd) != _pty_to_client.end()) {
        char buf[1024];
        int bytes = read(current_fd, buf, sizeof(buf));
        int client_fd = _pty_to_client[current_fd];

        if (bytes <= 0) {
          Logs::info() << "[Daemon] Process PTY closed during attach\n";
          _serv.sendData(client_fd, "\n[Process Terminated]\n");
          epoll_ctl(_epfd, EPOLL_CTL_DEL, current_fd, nullptr);
          _pty_to_client.erase(current_fd);
          _client_to_pty.erase(client_fd);
          close(client_fd);
        } else {
          write(client_fd, buf, bytes);
        }
      } else {
        std::string input = _serv.readData(current_fd, this->_epfd);
        std::string output;
        if (!input.empty()) {
          _cparser.setCommandParser(input);
          std::string cmd = _cparser.getCommand();
          std::vector<std::string> targets = _cparser.getParams();
          if (cmd == "fg") {
            if (targets.empty()) {
              _serv.sendData(current_fd,
                             "Error: fg requires a target process.\n");
            } else {
              Process *proc = _manager.getExactProcess(targets[0]);
              if (proc && proc->getPtyMaster() >= 0 &&
                  proc->getState() == ProcessState::RUNNING) {
                int pty_fd = proc->getPtyMaster();

                _client_to_pty[current_fd] = pty_fd;
                _pty_to_client[pty_fd] = current_fd;

                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLRDHUP;
                ev.data.fd = pty_fd;
                epoll_ctl(_epfd, EPOLL_CTL_ADD, pty_fd, &ev);
                _serv.sendData(current_fd, "ATTACH_OK\n", false);
                Logs::info() << "[Daemon] Client attached to PTY\n";
              } else {
                _serv.sendData(
                    current_fd,
                    "Error: Process not found, not running, or has no PTY\n");
              }
            }
          } else if (!cmd.empty()) {
            output = _manager.executeCommand((cmd), targets);
            _serv.sendData(current_fd, output);
          }
        }
      }
    }
    _manager.reap();
    _manager.updateRunningStates();
  }
}
