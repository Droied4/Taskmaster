#include "Daemon.hpp"
#include "Logs.hpp"
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

static void help(void) {
  std::cout << "\
--help			-h -- print this ussage message and exit\n\
--configuration		-c -- configuration file path\n\
--logfile 		-l -- logfile  path\n\
--loglevel 		-e -- log level\n\
--nodaemon 		-n -- run in the foreground\n\
--version 		-v -- print taskmasterd version number and exit\n\
";
}

static Logs::Level logLevel(std::string optarg) {
  if (optarg == "debug")
    return Logs::Level::LDEBUG;
  else if (optarg == "info")
    return Logs::Level::INFO;
  else if (optarg == "warning")
    return Logs::Level::WARNING;
  else if (optarg == "error")
    return Logs::Level::ERROR;
  else {
    std::cout << "Choose a valid level:\n\
	- debug\n\
	- info\n\
	- warning\n\
	- error\n";
    exit(0);
  }
}

static struct Config flagCases(int ac, char *av[]) {
  struct Config conf;

  const char *short_opts = "hnvc:l:e:";

  const option long_opts[] = {
      {"help", no_argument, nullptr, 'h'},
      {"configuration", required_argument, nullptr, 'c'},
      {"logfile", required_argument, nullptr, 'l'},
      {"loglevel", required_argument, nullptr, 'e'},
      {"nodaemon", no_argument, nullptr, 'n'},
      {"version", no_argument, nullptr, 'v'},
      {nullptr, 0, nullptr, 0}};

  int opt;
  int long_index = 0;

  while ((opt = getopt_long(ac, av, short_opts, long_opts, &long_index)) !=
         -1) {
    switch (opt) {
    case 'h':
      help();
      exit(0);
    case 'v':
      std::cout << "Taskmasterd v0.1\n";
      exit(0);
    case 'l':
      Logs::setFile(optarg);
      break;
    case 'e':
      Logs::setMinLevel(logLevel(optarg));
      break;
    case 'n':
      conf.daemonize = false;
      break;
    case 'c':
      conf.config_path = optarg;
      break;
    default:
      help();
      exit(1);
    }
  }

  if (optind < ac) {
    std::cerr << "./taskmasterd: invalid option --" << " '" << av[optind] << "'"
              << "\n";
    help();
    exit(1);
  }

  return conf;
}

bool deescalate_privileges(const char* target_user) {
    struct passwd* pwd = getpwnam(target_user);
    if (!pwd) {
		Logs::error() << "User: \'" << target_user << "\' Not Found." << std::endl;
        return false;
    }

    uid_t target_uid = pwd->pw_uid;
    gid_t target_gid = pwd->pw_gid;

    if (setgid(target_gid) != 0) {
		Logs::error() << "Change GID Failed" << std::endl;
        return false;
    }

    if (setuid(target_uid) != 0) {
		Logs::error() << "Change UID Failed." << std::endl;
        return false;
    }

    if (setuid(0) == 0 || getuid() == 0)
	{
        Logs::error() << "¡Security breach! de-escalation failed." << std::endl;
        return false;
	}
    return true;
}

int main(int ac, char *av[]) {
	if (getuid() != 0) {
		Logs::error() << "Must be init as root" << std::endl;
		return 1;
	}
	Daemon daemon(flagCases(ac, av));
	if (deescalate_privileges("taskmaster"))
		daemon.run();
	return 0;
}
