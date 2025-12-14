#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<ctime>
#include<string.h>
#include<cstdarg>
#include<mutex>
#include<queue>
#include<condition_variable>
#include<thread>

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

    void init(bool is_async, int max_size = 800);

    void max_size(int bytes = 1024*1024*1024);

    Logger(const Logger& tp) = delete;

    Logger& operator=(const Logger& tp) = delete;

private:

    ~Logger();

    Logger();

    bool rotate();

    void writer();

    std::queue<std::string> m_block_queue;
    std::thread m_writer;
    int m_size;
    int m_max;
    int m_queue_max_size;
    bool m_is_async;
    bool m_stop;
    Level m_level;
    std::string m_filename;
    std::ofstream m_fout;
    std::mutex m_write_mutex;
    std::mutex m_queue_mutex;
    std::condition_variable m_producer;
    std::condition_variable m_consumer;
    static const char* s_levels[LEVEL_COUNT];
};