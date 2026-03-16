#include "CommandParser.hpp"

CommandParser::CommandParser() {}

CommandParser::~CommandParser() {}

void CommandParser::setCommandParser(std::string input) {
  input = clearInput(input);
  this->_command = getFirstParam(input);
  this->_params = splitParams(input);
}

std::string CommandParser::getFirstParam(std::string input) {	
  size_t end = input.find(' ');

  std::string cmd = input.substr(0, end);
  std::cout << "cmd: " << cmd << "\n";

  return (cmd);
}

std::string CommandParser::clearInput(std::string input)
{
	auto it(input.begin());
	for (; it != input.end(); it++)
	{
		if (*it != ' ')
			break ;
	}
	input.erase(input.begin(), it);
	return (input);
}

std::vector<std::string> CommandParser::splitParams(std::string input) {
  std::vector<std::string> params;
  std::stringstream ss(input);
  std::string item;

  while (getline(ss, item, ' '))
  {
	  if (!item.empty())
		  params.push_back(item);
  }

  if (!params.empty())
	  params.erase(params.begin());
	std::cout << "params: ";
  for(auto it(params.begin()); it != params.end(); it++)
	  std::cout << *it;
  std::cout << "\n";
  return (params);
}

std::string CommandParser::getCommand() const { return (this->_command); }

std::vector<std::string> CommandParser::getParams() const {
  return (this->_params);
}
