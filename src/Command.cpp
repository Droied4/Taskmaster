#include "Command.hpp"

Command::Command(std::string name)
{
	this->_name = name;
}

Command::~Command()
{}

//Start a proccess
Start::Start() : Command("START") {}

void Start::execute(std::string param) 
{
	std::cout << "Start " << param << " proccess\n";
}

//Stop a proccess
Stop::Stop() : Command("STOP") {}

void Stop::execute(std::string param) 
{
	std::cout << "Stop " << param << " proccess\n";
}

//Restart a process
Restart::Restart() : Command("RESTART") {}

void Restart::execute(std::string param) 
{
	std::cout << "Restart " << param << " proccess\n";
}

//Reload the config file
Reload::Reload() : Command("RELOAD") {}

void Reload::execute(std::string param) 
{
	(void)param;
	std::cout << "Reload config file \n";
}

//Show the status of the proccess
Status::Status() : Command("STATUS") {}

void Status::execute(std::string param) 
{
	(void)param;
	std::cout << "show status of all proccess\n";
}

//Shutdown taskmasterd
Exit::Exit() : Command("EXIT") {}

void Exit::execute(std::string param) 
{
	(void)param;
	std::cout << "exit from supervisorctl\n";
}
//Shutdown taskmasterctl
Quit::Quit() : Command("QUIT") {}

void Quit::execute(std::string param) 
{
	(void)param;
	std::cout << "exit from supervisorctl\n";
}
//Display help
Help::Help() : Command("HELP") {}

void Help::execute(std::string param) 
{
 	if (param.empty())
		std::cout << "Available commands:\n"
            << "  start <name>   - Start a process\n"
            << "  stop <name>    - Stop a process\n"
            << "  restart <name> - Restart a process\n"
            << "  status         - Show process status\n"
            << "  reload         - Reload configuration file\n"
            << "  exit/quit      - Close this terminal\n";
}
