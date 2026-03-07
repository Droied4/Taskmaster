#include "Server.hpp"

Server::Server()
{
	this->_serv_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (this->_serv_fd < 0)
		exit(1); //error();exception //hablar con 
				 //tuti de como manejar los errores de este tipo
	//logs: server socket created
	std::cout << "Server Socket Created\n";
  	bzero(&this->_serv_addr, sizeof(this->_serv_addr));
  	this->_serv_addr.sun_family = AF_UNIX;
  	strncpy(this->_serv_addr.sun_path, this->SOCK_PATH, sizeof(this->_serv_addr.sun_path) - 1);
    this->_serv_addr.sun_path[sizeof(this->_serv_addr.sun_path) - 1] = '\0';

	this->_epfd = epoll_create1(0);
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = this->_serv_fd;

	epoll_ctl(this->_epfd, EPOLL_CTL_ADD, this->_serv_fd, &ev);
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
	unlink(SOCK_PATH);
	if (bind(this->_serv_fd, reinterpret_cast<sockaddr *>(&this->_serv_addr), sizeof(this->_serv_addr)) < 0)
		exit(1);
	//logs: server socket binded
	std::cout << "Server binded on route: " << SOCK_PATH << "\n"; 
	if (listen(this->_serv_fd, 0) < 0)
		exit(1);
	//logs: server listening 	
	std::cout << "Server is now listening\n";
}

void Server::readData(int fd)
{
	char buffer[BUFFER_SIZE + 1];
	bzero(buffer, BUFFER_SIZE + 1);
	int bytes;

	bytes = read(fd, buffer, BUFFER_SIZE);   
	if (bytes <= 0)
	{
		//logs: Client Disconnected!
        std::cout << "Client Disconnected : " << fd << "\n";
        epoll_ctl(this->_epfd, EPOLL_CTL_DEL, fd, nullptr);
		close(fd);
		return ;
	}
	buffer[bytes] = '\0';
	std::cout << "Data read: " << buffer;
}

void Server::acceptConnection()
{
	int client_socket;

	client_socket = accept(this->_serv_fd, nullptr, nullptr);
	if (client_socket < 0)	
		exit(1);

	int flags = fcntl(client_socket, F_GETFL, 0);
	fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = client_socket; 
	epoll_ctl(this->_epfd, EPOLL_CTL_ADD, client_socket, &ev);

	//logs: Client Connected!
	std::cout << "Client Connected: " << client_socket << "\n";
	//Server::sendData();
}

void Server::run()
{
	struct epoll_event events[EVENTS_SIZE];
	bindListen();
	while (42)
	{
	    int nfds = epoll_wait(this->_epfd, events, EVENTS_SIZE, -1);
		for (int i = 0; i < nfds; ++i)
		{
			if(events[i].data.fd == this->_serv_fd)
				acceptConnection();
			else
				readData(events[i].data.fd);
		}
	}
}
