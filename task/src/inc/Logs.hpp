#pragma once
#include <fstream>
#include <iostream>
#include <ostream>

class Logs {
public:
  Logs(const Logs &) = delete;
  Logs &operator=(const Logs &) = delete;
  ~Logs();

  enum class Level { ERROR = 0, WARNING, INFO, LDEBUG };

  static Logs &error();
  static Logs &warning();
  static Logs &info();
  static Logs &debug();
  Logs &operator<<(std::ostream &(*manip)(std::ostream &));
  template <typename T> Logs &operator<<(const T &value) {
    if (_enabled && _err && !_file.is_open())
      std::cerr << value;
    else if (_enabled)
      *_output << value;
    return (*this);
  }
  static void setMinLevel(Level level);
  static void setFile(std::string filename);

private:
  Logs();

  Level _min_level;
  bool _enabled;
  bool _err;
  std::ostream *_output;
  std::ofstream _file;

  void closeFile();
  void printTimeStamp() const;
  void printLevel(Level level) const;
  static Logs &getInstance();
  Logs &operator<<(Level level);
  void setEnable();
};
