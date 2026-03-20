#pragma once

#include <string>
#include <vector>

class CommandParser {
private:
  std::string _command;
  std::vector<std::string> _params;

  std::string getFirstParam(std::string input);
  std::vector<std::string> splitParams(std::string input);
  std::string clearInput(std::string input);

public:
  CommandParser();
  CommandParser(const CommandParser &) = delete;
  CommandParser &operator=(const CommandParser &) = delete;
  ~CommandParser();

  void setCommandParser(std::string input);
  std::string getCommand() const;
  std::vector<std::string> getParams() const;
};
