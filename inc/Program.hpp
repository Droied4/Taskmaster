#pragma once

#include <map>
#include <string>
#include <sys/types.h>
#include <vector>

// solo la declaracion para que no se me rompa todo xd
class Process;

struct ProgramConfig {
  std::string cmd;
  int numprocs;
  mode_t umask;
  std::string workingdir;
  bool autostart;
  std::string autorestart;
  std::vector<int> exitcodes;
  int startretries;
  int starttime;
  int stopsignal;
  int stoptime;
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
