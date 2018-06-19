#include"logger.hpp"





Logger* Logger::m_log = NULL;


Logger::~Logger(){
	getStream(INFO)<<std::endl<<std::flush;
	m_info_log_file.close();
}


Logger* Logger::getInstance(){
	if (m_log==NULL){
		m_log = new Logger();
		m_log->initLogger("info.txt");
	}
	return m_log;
}


ostream& Logger::start(log_level_t log_level, const string& file, const int line, const std::string& function){
	ostream& stm = getStream(log_level);
	if (log_level==INFO){
		return stm<<flush;
	}else if (log_level==ERROR){
		return stm<<"ERROR: "<<file<<" : "<<line<<" : "<<function<<endl<<flush;
	}else{
		return stm<<flush;
	}
}


void Logger::initLogger(const string& info_filename){
	m_info_log_file.open(info_filename.c_str());
	if (!m_info_log_file.is_open()){
		cout<<"failed to open "<<info_filename<<endl;
		return;
	}
}


std::ostream& Logger::getStream(log_level_t log_level){
	if (log_level == INFO){
		return m_info_log_file.is_open() ? m_info_log_file : cout;
	}else if (log_level == ERROR){
		return cerr;
	}else{
		return cout;
	}
}
