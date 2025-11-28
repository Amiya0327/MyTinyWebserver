#include "Webserver.h"

unsigned short port = 9006;

int main(int argc, char* argv[])
{
    Webserver server;
    if(argc>=2)
    port = atoi(argv[1]);

    server.init(port,0,0);

    server.TRIGmode(0);

    server.eventListen();
    
    server.eventLoop();

    return 0;
}
