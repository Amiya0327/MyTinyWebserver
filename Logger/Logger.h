#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<ctime>
#include<string.h>
#include<cstdarg>
#include<mutex>

#define LOG_DEBUG(format,...) Logger::get_instance().log(Logger::DEBUG,__FILE__,__LINE__,format,##__VA_ARGS__);
#define LOG_INFO(format,...) Logger::get_instance().log(Logger::INFO,__FILE__,__LINE__,format,##__VA_ARGS__);
#define LOG_WARN(format,...) Logger::get_instance().log(Logger::WARN,__FILE__,__LINE__,format,##__VA_ARGS__);
#define LOG_ERROR(format,...) Logger::get_instance().log(Logger::ERROR,__FILE__,__LINE__,format,##__VA_ARGS__);
#define LOG_FATAL(format,...) Logger::get_instance().log(Logger::FATAL,__FILE__,__LINE__,format,##__VA_ARGS__);

class Logger
{
public:
    enum Level
    {
        DEBUG = 0,
        INFO,
        WARN,
        ERROR,
        FATAL,
        LEVEL_COUNT
    };

    static Logger& get_instance();

    bool log(Level level,const char* file,int line,const char* format,...);

    bool open(const std::string& filename);

    void close();

    void level(Level level);

    void max_size(int bytes = 1024*1024*1024);

    Logger(const Logger& tp) = delete;

    Logger& operator=(const Logger& tp) = delete;

private:

    ~Logger();

    Logger();

    bool rotate();

    int m_size;
    int m_max;
    Level m_level;
    std::string m_filename;
    std::ofstream m_fout;
    std::mutex m_write_mutex;
    static const char* s_levels[LEVEL_COUNT];
};