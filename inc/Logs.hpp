#ifndef LOGS
# define LOGS

# include <iostream>
# include <ctime>
# include <iomanip>
# include <chrono>

class Logs 
{
	public: 
    	Logs(const Logs&) = delete;
    	Logs& operator=(const Logs&) = delete;
		~Logs();
	
		static Logs& error();	
		static Logs& warning();	
		static Logs& info();	
		static Logs& debug();
		Logs& operator<<(std::ostream& (*manip)(std::ostream&));
		template<typename T> Logs& operator<<(const T& value)
		{
			std::cout << value;
			return (*this);
		}
	private:
		Logs();

		enum class Level {
		ERROR,
		WARNING,
		INFO,
		DEBUG
		};
	
		void printTimeStamp() const;	
		void printLevel(Level level) const;
		static Logs& getInstance();
		Logs& operator<<(Level level);
};

#endif
