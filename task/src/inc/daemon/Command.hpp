#pragma once

#include "Program.hpp"
#include <memory>

class ProcessManager;

class Command {
protected:
  std::string _name;

public:
  Command(std::string name);
  Command(const Command &obj) = delete;
  Command &operator=(const Command &obj) = delete;
  virtual ~Command();

  virtual std::string
  execute(std::map<std::string, std::unique_ptr<Program>> &programs,
          const std::vector<std::string> &target) = 0;
};

class Start : public Command {
public:
  Start();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class Stop : public Command {
public:
  Stop();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class Reload : public Command {
public:
  Reload();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class Status : public Command {
public:
  Status();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class Restart : public Command {
public:
  Restart();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class Pid : public Command {
public:
  Pid();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class Shutdown : public Command {
public:
  Shutdown();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class Help : public Command {
public:
  Help();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class GetPrograms : public Command {
public:
  GetPrograms();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class GetCommands : public Command {
public:
  GetCommands();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};

class Fg : public Command {
public:
  Fg();
  std::string execute(std::map<std::string, std::unique_ptr<Program>> &programs,
                      const std::vector<std::string> &target) override;
};
