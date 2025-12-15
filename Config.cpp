#include "Config.h"

Config::Config()
{
    m_port = 8080; //默认端口

    m_trig_mode = 3; //触发模式，默认ET+ET

    m_log_mode = 1; //日志模式，默认异步

    m_log_close = 1; //日志，默认关闭

    m_thread_num = 0; //线程池线程数量，默认关闭线程池
}

void Config::parse_arg(int argc,char* argv[])
{
    int opt;
    const char* str = "p:m:l:c:t";

    while((opt = getopt(argc,argv,str))!=-1)
    {
        if(opt=='p')
        {
            m_port = atoi(optarg);
        }
        else if(opt=='m')
        {
            m_trig_mode = atoi(optarg);
        }
        else if(opt=='l')
        {
            m_log_mode = atoi(optarg);
        }
        else if(opt=='c')
        {
            m_log_close = atoi(optarg);
        }
        else if(opt=='t')
        {
            m_thread_num = atoi(optarg);
        }
        else
        {
            continue;
        }
    }
}