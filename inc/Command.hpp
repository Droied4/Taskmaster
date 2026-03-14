#pragma once

#include <iostream>
#include "Logs.hpp"

class Command {
	private:
		bool _connected;	
	protected:
		std::string _name;
	public:
		Command(std::string name);
		Command(const Command& obj) = delete;
		Command& operator=(const Command& obj) = delete;
		virtual ~Command();

		virtual void execute(std::string param) = 0;
};

class Start : public Command
{
	public:
		Start();
		void execute(std::string param);
};

class Stop : public Command
{
	public:
		Stop();
		void execute(std::string param);
};

class Reload : public Command
{
	public:
		Reload();
		void execute(std::string param);
};

class Status : public Command
{
	public:
		Status();
		void execute(std::string param);
};

class Restart : public Command
{
	public:
		Restart();
		void execute(std::string param);
};

class Exit : public Command
{
	public:
		Exit();
		void execute(std::string param);
};

class Quit : public Command
{
	public:
		Quit();
		void execute(std::string param);
};

class Help : public Command
{
	public:
		Help();
		void execute(std::string param);
};
