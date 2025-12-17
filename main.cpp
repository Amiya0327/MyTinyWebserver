#include "Webserver.h"
#include "Config.h"

int main(int argc, char* argv[])
{
    std::string host = "localhost";
    unsigned short sqlport = 3306;
    std::string user = "yzl";
    std::string passwd = "qw123456";
    std::string dbname = "webserver";

    Config config;
    config.parse_arg(argc,argv); //解析命令行参数
    Webserver server;

    server.init(config.m_port,config.m_trig_mode,config.m_log_mode,
        config.m_log_close,config.m_thread_num,
        host,sqlport,user,passwd,dbname); //初始化

    server.sqlPool(); //数据库连接池

    server.threadPool(); //线程池

    server.TRIGmode(); //触发模式

    server.Log("Server.log"); //日志

    server.eventListen(); //监听
     
    server.eventLoop(); //主循环

    return 0;
}
