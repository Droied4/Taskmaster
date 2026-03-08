#include "Server.hpp"
#include "Logs.hpp"

int main(int ac, char *av[]) 
{
  if (ac >= 1 && av[1])
  {
		 std::string level = av[1];

        if (level == "DEBUG")
            Logs::setMinLevel(Logs::Level::DEBUG);
        else if (level == "INFO")
            Logs::setMinLevel(Logs::Level::INFO);
        else if (level == "WARNING")
            Logs::setMinLevel(Logs::Level::WARNING);
        else if (level == "ERROR")
            Logs::setMinLevel(Logs::Level::ERROR);
  } 
  Server serv;
  serv.run();

  return (0);
}
