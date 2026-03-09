#pragma once

#include <map>
#include <string>
#include <sys/types.h>
#include <vector>

// solo la declaracion para que no se me rompa todo xd
class Process;

struct ProgramConfig {
  std::string cmd = "";
  int numprocs = 1;
  mode_t umask = 022;
  std::string workingdir = "";
  bool autostart = false;
  std::string autorestart = "unexpected";
  std::vector<int> exitcodes = {0};
  int startretries = 3;
  int starttime = 1;
  int stopsignal = 15;
  int stoptime = 10;
  std::string stdout_path;
  std::string stderr_path;
  std::map<std::string, std::string> env;
};

class Program {
private:
  std::string _name;
  ProgramConfig _config;
  std::vector<Process *> _processes;

public:
  Program(const std::string &name, const ProgramConfig &config);
  ~Program();

  // con esto evitamos que se puedan copiar o asignar programas. cada Program es
  // unico
  Program(const Program &) = delete;
  Program &operator=(const Program &) = delete;

  void start();
  void stop();
  void restart();
  void monitor(); // verificar estado de los hijos y aplicar el autorestart si
                  // es necesario

  const std::string &getName() const;
  const ProgramConfig &getConfig() const;
};
