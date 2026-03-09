#pragma once

#include <iostream>
#include "Server.hpp"
#include "Logs.hpp"
#include <sys/epoll.h>

class Daemon {
	private:
		int _epfd;
		Server _serv;
	
	public:
		Daemon();
		Daemon(const Daemon& obj) = delete;
		Daemon& operator=(const Daemon& obj) = delete;
		~Daemon();

  		static constexpr int EVENTS_SIZE = 64;

  		void run();
};
