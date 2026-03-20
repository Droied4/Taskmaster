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

static Logs::Level logLevel(std::string optarg)
{
	if (optarg == "debug")
		return Logs::Level::LDEBUG;
	else if (optarg == "info")
		return Logs::Level::INFO;
	else if (optarg == "warning")
		return Logs::Level::WARNING;
	else if (optarg == "error")
		return Logs::Level::ERROR;
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

static struct Config flagCases(int ac, char *av[])
{
	struct Config conf;

	const char* short_opts = ":?hnvc:l:e:";

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
				conf.logfile = optarg;
				break ;
			case 'e':
				conf.loglevel = logLevel(optarg);
				break ;
			case 'n':
				conf.daemonize = false;
				break ;
			case 'c'
				conf.config_path = optarg;
				break ;
		}
	}
}

int main(int ac, char *av[]) {

  struct Config conf;
  conf = flagCases(ac, av);
  Logs::setFile(conf.logfile);
  Logs::setMinLevel(conf.loglevel);

  ProcessManager manager(conf.config_path);
  Daemon daemon(manager);

  daemon.run(); 
  return 0;
}
