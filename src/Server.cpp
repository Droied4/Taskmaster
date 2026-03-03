#include "Server.hpp"

Server::Server()
{
	this->_serv_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (!this->_serv_fd)
		exit(1); //error();exception //hablar con 
				 //tuti de como manejar los errores de este tipo
	//logs: server socket created
  	bzero(&this->_serv_addr, sizeof(this->_serv_addr));
  	this->_serv_addr.sun_family = AF_UNIX;
  	strncpy(this->_serv_addr.sun_path, this->SOCK_PATH, sizeof(this->_serv_addr.sun_path) - 1);
    this->_serv_addr.sun_path[sizeof(this->_serv_addr.sun_path) - 1] = '\0'; // null-terminar
}

Server::Server(const Server &obj)
{
	*this = obj;
}

Server &Server::operator=(const Server &obj)
{
	if (this != &obj)
		this->_serv_fd = obj.getServerFd();
	return (*this);
}

Server::~Server()
{
	close(this->_serv_fd);
}

void Server::setServerFd(int fd)
{
	this->_serv_fd = fd;
}

int Server::getServerFd() const
{
	return (this->_serv_fd);
}

void Server::bindListen()
{
	if (!bind(this->_serv_fd, reinterpret_cast<sockaddr *>(&this->_serv_addr), sizeof(this->_serv_addr)))
		exit(1);
	//logs: server socket binded
	if (!listen(this->_serv_fd, 0))
		exit(1);
	//logs: server listening 	
}

void Server::readData()
{
	char buffer[BUFFER_SIZE + 1];

	for(std::vector<int>::iterator it(this->_client_fd.begin()); it < this->_client_fd.end(); ++it)
	{
		if (!read(*it, buffer, BUFFER_SIZE))
			exit(1); //manage errno and close connection
	}
}

void Server::acceptConnection()
{
	int client_socket;

	while (42)
	{
		client_socket = accept(this->_serv_fd, nullptr, nullptr);
		if (!client_socket)	
			exit(1);

		int flags = fcntl(client_socket, F_GETFL, 0);
    	fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

		this->_client_fd.push_back(client_socket);
		//logs: New connection accepted!
		//select logic waiting for data to be read
		Server::readData();
		//Server::sendData();
	}
}
