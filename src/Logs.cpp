#include "Logs.hpp"

Logs::Logs() {
  this->_min_level = Level::INFO;
  this->_enabled = true;
}

Logs::~Logs() {}

Logs &Logs::getInstance() {
  static Logs instance;
  return (instance);
}

void Logs::printTimeStamp() const {
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::tm *parts = std::localtime(&now_c);

  std::cout << std::put_time(parts, "[%Y-%m-%d %H:%M:%S] ");
}

void Logs::printLevel(Level level) const {
  switch (level) {
  case Level::ERROR:
    std::cout << "\033[31m[ERROR] \033[0m";
    break;
  case Level::WARNING:
    std::cout << "\033[33m[WARNING] \033[0m";
    break;
  case Level::INFO:
    std::cout << "\033[34m[INFO] \033[0m";
    break;
  case Level::LDEBUG:
    std::cout << "\033[32m[DEBUG] \033[0m";
    break;
  }
}

void Logs::setEnable() { this->_enabled = true; }

void Logs::setMinLevel(Level level) {
  Logs &logger = getInstance();
  logger._min_level = level;
  logger.setEnable();
}

Logs &Logs::operator<<(Level level) {
  if (_min_level >= level) {
    _enabled = true;
    printTimeStamp();
    printLevel(level);
  } else
    _enabled = false;
  return (*this);
}

Logs &Logs::operator<<(std::ostream &(*manip)(std::ostream &)) {
  if (_enabled)
    manip(std::cout);
  return *this;
}

Logs &Logs::error() { return (getInstance() << Level::ERROR); }

Logs &Logs::warning() { return (getInstance() << Level::WARNING); }

Logs &Logs::info() { return (getInstance() << Level::INFO); }

Logs &Logs::debug() { return (getInstance() << Level::LDEBUG); }
