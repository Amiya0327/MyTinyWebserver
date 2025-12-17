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

    bool log(Level level,const char* file,int line,const char* format,...); //写入日志

    bool open(const std::string& filename); //打开文件

    void close(); //关闭文件

    void level(Level level); //日志最低打印级别

    void init(bool is_async,bool log_close = 0, int max_size = 800); 

    void max_size(int bytes = 1024*1024*1024); //单个日志最大字节数

    Logger(const Logger& tp) = delete; 

    Logger& operator=(const Logger& tp) = delete;

private:

    ~Logger();

    Logger();

    bool rotate(); //滚动日志

    void writer();

    std::queue<std::string> m_block_queue; //阻塞队列
    std::thread m_writer; //异步写入日志线程
    int m_size; //当前文件大小
    int m_max; //单个文件最大大小
    int m_queue_max_size; //阻塞队列最大大小
    bool m_is_async; //是否异步
    bool m_stop;  //异步线程停止标志
    bool m_log_close; //是否开启日志
    Level m_level;
    std::string m_filename;
    std::ofstream m_fout;
    static const char* s_levels[LEVEL_COUNT];

    //异步日志锁
    std::mutex m_write_mutex;
    std::mutex m_queue_mutex;
    std::condition_variable m_producer;
    std::condition_variable m_consumer;
};