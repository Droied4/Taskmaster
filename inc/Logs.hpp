#ifndef LOGS
#define LOGS

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

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
    if (_enabled && _err)
		std::cerr << value;
	else if (_enabled)
		std::cout << value;
    return (*this);
  }
  static void setMinLevel(Level level);

private:
  Logs();

  Level _min_level;
  bool _enabled;
  bool _err;

  void printTimeStamp() const;
  void printLevel(Level level) const;
  static Logs &getInstance();
  Logs &operator<<(Level level);
  void setEnable();
};

#endif
