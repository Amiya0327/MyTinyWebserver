#include<unistd.h>
#include<stdlib.h>

class Config
{
public:

    Config();

    void parse_arg(int argc,char* argv[]); //命令行参数解析

    int m_port;
    int m_trig_mode;
    int m_log_mode;
    int m_log_close;
    int m_thread_num;

};