#pragma once 
#include <fcntl.h>
#include<sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include<string.h>

class Utils
{
public:
    Utils();
    ~Utils();

    //设置非阻塞
    void setnonblocking(int fd);

    //发送错误信息
    void show_error(int fd,const char* info);

    //将内核事件表注册读事件
    void addfd(int epollfd, int fd, bool one_shot,bool TRIGmode);

    void removefd(int epollfd,int fd);
private:
};