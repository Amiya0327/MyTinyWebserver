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
    config.parse_arg(argc,argv);
    Webserver server;

    server.init(config.m_port,config.m_trig_mode,config.m_log_mode,
        config.m_log_close,config.m_thread_num,
        host,sqlport,user,passwd,dbname);

    server.sqlPool();

    server.threadPool();

    server.TRIGmode();

    server.Log("Server.log");

    server.eventListen();
    
    server.eventLoop();

    return 0;
}
