#include "Timer.h"

Utils::Utils()
{

}

Utils::~Utils()
{

}

void Utils::setnonblocking(int fd)
{
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);
}
    
void Utils::addfd(int epollfd, int fd,bool one_shot,bool TRIGmode)
{
    epoll_event ev;
    ev.data.fd = fd;
    if(TRIGmode)
        ev.events = EPOLLIN|EPOLLET|EPOLLRDHUP;
    else
        ev.events = EPOLLIN|EPOLLRDHUP;

    if(one_shot)
    ev.events|= EPOLLONESHOT;
    setnonblocking(fd);
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
}

void Utils::show_error(int fd,const char* info)
{
    //代码待完善，包含http协议信息
    send(fd,info,strlen(info),0);
    close(fd);
}
