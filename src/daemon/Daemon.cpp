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
      _is_shutting_down(false), _manager(conf.config_path), _serv(_epfd) {
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

void Daemon::handlePTYInput(int client_fd) {
  char buf[1024];
  int bytes = read(client_fd, buf, sizeof(buf) - 1);
  int pty_fd = _client_to_pty[client_fd];

  if (bytes <= 0) {
    Logs::info() << "[Daemon] Client detached from PTY\n";
    epoll_ctl(_epfd, EPOLL_CTL_DEL, pty_fd, nullptr);
    epoll_ctl(_epfd, EPOLL_CTL_DEL, client_fd, nullptr);
    _client_to_pty.erase(client_fd);
    _pty_to_client.erase(pty_fd);
    close(client_fd);
  } else {
    buf[bytes] = '\0';
    std::string data(buf, bytes);

    handlePTYResize(pty_fd, data);

    if (!data.empty()) {
      write(pty_fd, data.c_str(), data.length());
    }
  }
}
void Daemon::handlePTYOutput(int pty_fd) {
  char buf[1024];
  int bytes = read(pty_fd, buf, sizeof(buf));
  int client_fd = _pty_to_client[pty_fd];

  if (bytes <= 0) {
    Logs::info() << "[Daemon] Process PTY closed during attach\n";
    _serv.sendData(client_fd, "\r\n[Process Terminated]\r\n");
    epoll_ctl(_epfd, EPOLL_CTL_DEL, pty_fd, nullptr);
    epoll_ctl(_epfd, EPOLL_CTL_DEL, client_fd, nullptr);
    _pty_to_client.erase(pty_fd);
    _client_to_pty.erase(client_fd);
    close(client_fd);
  } else {
    write(client_fd, buf, bytes);
  }
}

void Daemon::handleSignal() {
  struct signalfd_siginfo fdsi;

  ssize_t s = read(_sig_fd, &fdsi, sizeof(struct signalfd_siginfo));

  if (s == sizeof(struct signalfd_siginfo)) {
    if (fdsi.ssi_signo == SIGHUP) {
      Logs::info() << "[Daemon] Received SIGHUP, reloading configuration..."
                   << std::endl;
      _manager.reloadConfig();
    } else if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
      Logs::info() << "[Daemon] Received signal to terminate, shutting down..."
                   << std::endl;
      _is_shutting_down = true;

      for (auto const &[client_fd, pty_fd] : _client_to_pty) {
        _serv.sendData(client_fd,
                       "\r\n[Daemon shutting down. Detaching...]\r\n");
        epoll_ctl(_epfd, EPOLL_CTL_DEL, client_fd, nullptr);
        epoll_ctl(_epfd, EPOLL_CTL_DEL, pty_fd, nullptr);
        close(client_fd);
      }

      _client_to_pty.clear();
      _pty_to_client.clear();
      _manager.shutdownAll();
    }
  }
}

void Daemon::handlePTYResize(int pty_fd, std::string &data) {
  size_t pos = data.find("\x1E"
                         "WINCH:");

  while (pos != std::string::npos) {
    size_t end = data.find("\x1E", pos + 1);
    if (end != std::string::npos) {
      std::string winch_cmd = data.substr(pos + 7, end - pos - 7);
      size_t sep = winch_cmd.find(';');

      if (sep != std::string::npos) {
        struct winsize ws;
        ws.ws_row = std::stoi(winch_cmd.substr(0, sep));
        ws.ws_col = std::stoi(winch_cmd.substr(sep + 1));
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;

        ioctl(pty_fd, TIOCSWINSZ, &ws);
      }
      data.erase(pos, end - pos + 1);
    } else {
      break;
    }
    pos = data.find("\x1E"
                    "WINCH:");
  }
}

void Daemon::processClientCommand(int client_fd, const std::string &input) {

  if (_is_shutting_down) {
    _serv.sendData(
        client_fd,
        "Error: Daemon is shutting down. No new commands accepted.\n");
    return;
  }

  _cparser.setCommandParser(input);
  std::string cmd = _cparser.getCommand();
  std::vector<std::string> targets = _cparser.getParams();

  if (cmd.empty())
    return;

  std::string output = _manager.executeCommand(cmd, targets);

  if (output.find("INTERNAL_ATTACH:") == 0) {
    std::string pty_info = output.substr(16);
    size_t separator = pty_info.find(':');

    int pty_fd = std::stoi(pty_info.substr(0, separator));
    std::string proc_name = pty_info.substr(separator + 1);

    if (_pty_to_client.find(pty_fd) != _pty_to_client.end()) {
      _serv.sendData(client_fd,
                     "Error: PTY already attached to another client.\n");
      return;
    }

    _client_to_pty[client_fd] = pty_fd;
    _pty_to_client[pty_fd] = client_fd;

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.fd = pty_fd;
    epoll_ctl(_epfd, EPOLL_CTL_ADD, pty_fd, &ev);

    Logs::info() << "[Daemon] Client attached to PTY for process: " << proc_name
                 << "\n";

    _serv.sendData(client_fd, "ATTACH_OK\n", false);
  } else {
    _serv.sendData(client_fd, output);
  }
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
    int timeout = _is_shutting_down ? 10 : 100;
    int nfds = epoll_wait(_epfd, events, EVENTS_SIZE, timeout);
    for (int i = 0; i < nfds; ++i) {
      int current_fd = events[i].data.fd;

      if (current_fd == _serv.getServerFd()) {
        _serv.acceptConnection(_epfd);
      } else if (current_fd == _sig_fd) {
        handleSignal();
      } else if (_client_to_pty.find(current_fd) != _client_to_pty.end()) {
        handlePTYInput(current_fd);
      } else if (_pty_to_client.find(current_fd) != _pty_to_client.end()) {
        handlePTYOutput(current_fd);
      } else {
        std::string input = _serv.readData(current_fd, this->_epfd);
        if (!input.empty()) {
          processClientCommand(current_fd, input);
        }
      }
    }
    _manager.reap();
    _manager.updateRunningStates();
    if (_is_shutting_down && !_manager.hasActiveProcesses()) {
      Logs::info() << "[Daemon] All processes terminated. Exiting cleanly.\n";
      break;
    }
  }
}
