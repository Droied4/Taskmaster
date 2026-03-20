#include "Daemon.hpp"
#include "Logs.hpp"
#include "ProcessManager.hpp"
#include <getopt.h>

void help(void)
{
	std::cout << "--help \t\t-h -- print this ussage message and exit\n\
		--configuration \t\t-c -- configuration file path\n\
		--logfile \t\t-l -- logfile  path\n\
		--loglevel \t\t-e -- log level\n\
		--nodaemon \t\t-n -- run in the foreground\n\
		--version \t\t-v -- print taskmasterd version number and exit\n\
		\n";
}

static void flagCases(int ac, char *av[])
{
	std::vector<std::string> configs;

	const char* short_opts = ":?h";

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
			default:
				std::cerr << "Uso: " << argv[0] << " [-c config]\n";
				return 1;
		}
	}
}

int main(int ac, char *av[]) {
#ifdef DEBUG
  Logs::setMinLevel(Logs::Level::LDEBUG);
#endif
	
  flagCases(ac, av);
  Logs::setFile("/home/droied/42/taskmaster/logs/logs.txt");
  std::string config_path = "config.lua";
  if (argc > 1) {
    config_path = argv[1];
  }

  ProcessManager manager(config_path);

  Daemon daemon(manager);

  daemon.run();

  return 0;
}
