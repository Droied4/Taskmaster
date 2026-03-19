#include "Daemon.hpp"
#include "Logs.hpp"
#include "ProcessManager.hpp"
#define COMMON_OPTSTR= ":?h"

void help(void)
{
	std::cout << "Taskmasterd    \
		-h display help\n 		 \
		\n";
}

static void flagCases(int ac, char *av[])
{
	int ch;
	opterr = 0;
	while ((ch = getopt(ac, av, COMMON_OPTSTR)) != EOF) 
	{
		switch (ch)
		{
			case ':':
				exit(1);
				break ;
			case '?':
				help();
				break ;
			case 'h':
				help();
				break ;	
		}
	}
}

int main(int argc, char *argv[]) {
#ifdef DEBUG
  Logs::setMinLevel(Logs::Level::LDEBUG);
#endif
	
  flagCases(int ac, char **av);
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
