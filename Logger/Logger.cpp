#include "Logger.h"

Logger::~Logger()
{
    close();
}

Logger::Logger()
{
    m_level = DEBUG;
    m_size = 0;
    m_max = 0;
}

const char* Logger::s_levels[LEVEL_COUNT] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

Logger& Logger::get_instance()
{
    static Logger log;
    return log;
}

bool Logger::open(const std::string& filename)
{
    close();
    std::ifstream check_size(filename,std::ios::ate|std::ios::binary);
    m_size = check_size.tellg();
    close();
    m_fout.open(filename,std::ios::app|std::ios::out);
    if(!m_fout.is_open())
    return false;
    m_filename = filename;
    return true;
}

void Logger::close()
{
    m_fout.close();
}

bool Logger::log(Level level,const char* file,int line,const char* format,...)
{
    if(level<m_level)
    return false;
    if(!m_fout.is_open())
    return false;

    time_t t = time(0);
    struct tm tm_buf;
    struct tm* ptm = localtime_r(&t,&tm_buf);
    char timestamp[32] = {0};
    strftime(timestamp,sizeof(timestamp),"%Y-%m-%d %H:%M:%S",ptm);

    va_list args;
    va_start(args,format);
    int size = vsnprintf(nullptr,0,format,args);
    va_end(args);
    std::string s;
    file = strrchr(file,'/');
    file++;
    if(size>0)
    {
        char message[size+1] = {0};
        va_start(args,format);
        vsnprintf(message,sizeof(message),format,args);
        va_end(args);

        s = (std::string)"[" + timestamp + "] " 
        +"[" + s_levels[level] +"] " 
        +"[" + file + ":" + std::to_string(line) + "] "
        +message+"\n";
    }

    {
        std::lock_guard<std::mutex> lock(m_write_mutex);
        m_fout << s;
        m_fout.flush();
    
        m_size += s.size();
        if(m_size>=m_max&&m_max>0)
        {
            rotate();
        }
    }
    return true;
}

void Logger::level(Level level)
{
    m_level = level;
}

void Logger::max_size(int bytes)
{
    m_max = bytes;
}

bool Logger::rotate()
{
    close();
    time_t t = time(0);
    struct tm* ptm = localtime(&t);
    char timestamp[32] = {0};
    strftime(timestamp,sizeof(timestamp),"%Y-%m-%d_%H:%M:%S.",ptm);
    std::string new_name = timestamp+m_filename;
    if(rename(m_filename.c_str(),new_name.c_str())!=0)
    return false;

    open(m_filename);

    return true;
}