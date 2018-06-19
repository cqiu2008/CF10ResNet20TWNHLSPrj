#ifndef __LOGGER__H__
#define __LOGGER__H__

#include<iostream>
#include<iomanip>
#include<fstream>
#include<string>
#include<cstdlib>
#include<stdint.h>
#include<cstdlib>
#include<ctime>
#include "myostream.hpp"

using namespace std;


enum log_level_t{
	NONE=0,
	INFO,
	ERROR,
	CONSOLE
};


class Logger{
public:
	~Logger();
	static Logger* getInstance();
	ostream& start(log_level_t level, const string& file, const int line, const string& function);

private:
	static Logger* m_log;

	log_level_t m_log_level;

	ofstream m_info_log_file;

	Logger(){m_log_level = INFO;}
	Logger(const Logger& log){m_log_level = INFO;}
	Logger& operator=(const Logger& log){return *m_log;}

	std::ostream& getStream(log_level_t level);
	Logger(log_level_t level) : m_log_level(level) {};
	void initLogger(const string& info_filename);
};




#define LOG(log_level)  Logger::getInstance()->start(log_level,__FILE__,__LINE__,__FUNCTION__)
#define LOGNO(log_level) myout

#endif
