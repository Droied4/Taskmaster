#pragma once

#include "Logs.hpp"
#include "Program.hpp"
#include <iostream>

class ProcessManager;

class Command {
protected:
  std::string _name;

public:
  Command(std::string name);
  Command(const Command &obj) = delete;
  Command &operator=(const Command &obj) = delete;
  virtual ~Command();

  virtual std::string execute(std::map<std::string, Program *> &programs,
                              const std::string &target) = 0;
};

class Start : public Command {
public:
  Start();
  std::string execute(std::map<std::string, Program *> &programs,
                      const std::string &target) override;
};

class Stop : public Command {
public:
  Stop();
  std::string execute(std::map<std::string, Program *> &programs,
                      const std::string &target) override;
};

class Reload : public Command {
public:
  Reload();
  std::string execute(std::map<std::string, Program *> &programs,
                      const std::string &target) override;
};

class Status : public Command {
public:
  Status();
  std::string execute(std::map<std::string, Program *> &programs,
                      const std::string &target) override;
};

class Restart : public Command {
public:
  Restart();
  std::string execute(std::map<std::string, Program *> &programs,
                      const std::string &target) override;
};

class Shutdown : public Command {
public:
  Shutdown();
  std::string execute(std::map<std::string, Program *> &programs,
                      const std::string &target) override;
};
