#include "Logs.hpp"
#include <chrono>

Logs::Logs() : _output(&std::cout) {
  this->_min_level = Level::INFO;
  this->_enabled = true;
  this->_err = false;
}

Logs::~Logs() { closeFile(); }

Logs &Logs::getInstance() {
  static Logs instance;
  return (instance);
}

void Logs::printTimeStamp() const {
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::tm *parts = std::localtime(&now_c);

  *_output << std::put_time(parts, "[%Y-%m-%d %H:%M:%S] ");
}

void Logs::printLevel(Level level) const {
  switch (level) {
  case Level::ERROR:
    if (_output == &std::cout)
      *_output << "\033[31m[ERROR] \033[0m";
    else
      *_output << "[ERROR]";
    break;
  case Level::WARNING:
    if (_output == &std::cout)
      *_output << "\033[33m[WARNING] \033[0m";
    else
      *_output << "[WARNING]";
    break;
  case Level::INFO:
    if (_output == &std::cout)
      *_output << "\033[34m[INFO] \033[0m";
    else
      *_output << "[INFO]";
    break;
  case Level::LDEBUG:
    if (_output == &std::cout)
      *_output << "\033[32m[DEBUG] \033[0m";
    else
      *_output << "[DEBUG]";
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
  if (level == Level::ERROR)
    _err = true;
  else
    _err = false;
  return (*this);
}

Logs &Logs::operator<<(std::ostream &(*manip)(std::ostream &)) {
  if (_enabled)
    manip(*_output);
  return *this;
}

void Logs::setFile(std::string filename) {
  Logs &logger = getInstance();

  logger._file.open(filename, std::ios::app);
  if (logger._file.is_open())
    logger._output = &logger._file;
  else
    Logs::warning() << "path: " << filename
                    << " not found. Using default output\n";
}

void Logs::closeFile() {
  if (_file.is_open())
    _file.close();
}

Logs &Logs::error() { return (getInstance() << Level::ERROR); }

Logs &Logs::warning() { return (getInstance() << Level::WARNING); }

Logs &Logs::info() { return (getInstance() << Level::INFO); }

Logs &Logs::debug() { return (getInstance() << Level::LDEBUG); }
