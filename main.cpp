#include "Webserver.h"

unsigned short port = 9006;

int main(int argc, char* argv[])
{
    std::string host = "localhost";
    unsigned short sqlport = 3306;
    std::string user = "yzl";
    std::string passwd = "qw123456";
    std::string dbname = "webserver";
    Webserver server;
    if(argc>=2)
    port = atoi(argv[1]);

    server.init(port,3,host,sqlport,user,passwd,dbname);

    server.sqlPool();

    server.TRIGmode();

    server.Log("Server.log");

    server.eventListen();
    
    server.eventLoop();

    return 0;
}
