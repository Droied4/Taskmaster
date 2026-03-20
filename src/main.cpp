#include "Daemon.hpp"
#include "Logs.hpp"
#include "ProcessManager.hpp"
#include <getopt.h>

static void help(void)
{
	std::cout << "\
--help			-h -- print this ussage message and exit\n\
--configuration		-c -- configuration file path\n\
--logfile 		-l -- logfile  path\n\
--loglevel 		-e -- log level\n\
--nodaemon 		-n -- run in the foreground\n\
--version 		-v -- print taskmasterd version number and exit\n\
\n";
}

static void logLevel(std::string optarg)
{
	if (optarg == "debug")
  		Logs::setMinLevel(Logs::Level::LDEBUG);
	else if (optarg == "info")
  		Logs::setMinLevel(Logs::Level::INFO);
	else if (optarg == "warning")
  		Logs::setMinLevel(Logs::Level::WARNING);
	else if (optarg == "error")
  		Logs::setMinLevel(Logs::Level::ERROR);
	else
	{
		std::cout << "Choose a valid level:\n\
	- debug\n\
	- info\n\
	- warning\n\
	- error\n";
		exit(1);
	}
		
}

static void flagCases(int ac, char *av[], Daemon daemon)
{
	std::vector<std::string> configs;

	const char* short_opts = ":?hvc:l:e:";

	const option long_opts[] = {
		{"help", no_argument, nullptr, 'h'},
		{"configuration", required_argument, nullptr, 'c'},
		{"logfile", required_argument, nullptr, 'l'},
		{"loglevel", required_argument, nullptr, 'e'},
		{"nodaemon", no_argument, nullptr, 'n'},
		{"version", no_argument, nullptr, 'v'},
		{nullptr, 0, nullptr, 0}
	};

	int opt;
	int long_index = 0;

	while ((opt = getopt_long(ac, av, short_opts, long_opts, &long_index)) != -1) {
		switch (opt) {
			case 'h':
				help();
				exit(1);
			case 'v':
				std::cout << "Taskmasterd v0.1\n";
				exit(1);
			case 'l':
				Logs::setFile(optarg);
				break ;
			case 'e':
				logLevel(optarg);
				break ;
			case 'n':
				daemon.setDaemon(false);
				break ;
		}
	}
}

int main(int ac, char *av[]) {
	
  std::string config_path = "config.lua";
  ProcessManager manager(config_path);
  Daemon daemon(manager);
  
  flagCases(ac, av, daemon);

  daemon.run(); 
  return 0;
}
