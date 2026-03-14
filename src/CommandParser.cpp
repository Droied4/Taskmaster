#include  "CommandParser.hpp"

CommandParser::CommandParser()
{}

CommandParser::~CommandParser() {}

void CommandParser::setCommandParser(std::string input)
{
	this->_command = getFirstParam(input);
	this->_params = splitParams(input);
	Logs::info() << "Command: " << this->_command << "\n";
	Logs::info() << "Params: ";
	for (auto it(this->_params.begin()); it != this->_params.end(); it++)
		std::cout << *it << " ";
	std::cout << "\n";
}

std::string CommandParser::getFirstParam(std::string input)
{
	size_t end = input.find(' '); 
	
	std::string cmd = input.substr(0, end);

	return (cmd);
}

std::vector<std::string> CommandParser::splitParams(std::string input)
{
	std::vector<std::string> params;
	std::stringstream ss(input);
	std::string item;

	while(getline(ss, item, ' '))
		params.push_back(item);

	params.erase(params.begin());
	return (params);	
}

std::string CommandParser::getCommand()	const
{
	return(this->_command);	
}

std::vector<std::string> CommandParser::getParams() const	
{
	return(this->_params);	
}
