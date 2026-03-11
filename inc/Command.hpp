#pragma once

#include <iostream>
#include "Logs.hpp"

class Command {
	private:
	
	public:
		Command();
		Command(const Command& obj) = delete;
		Command& operator=(const Command& obj) = delete;
		~Command();
};
