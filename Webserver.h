#pragma once
#include<iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "HttpConn.h"
#include "Timer.h"

const int MAX_EVENT_NUM = 10000; //最大事件数
const int MAX_FD = 65536; //最大客户连接数
const int MAX_USER_NUM = 10000;

class Webserver
{
public:
    Webserver();

    ~Webserver();

    //配置模式初始化
    void init(unsigned short port,int lismode,int m_Climode);

    void TRIGmode(int mode);

    //事件循环
    void eventLoop();

    //监听初始化
    void eventListen();

private:

    void dealWithConn();
    void dealWithRead(int clifd);
    void dealWithWrite(int clifd);
    void dealWithClose(int clifd);

    int m_lismode;
    int m_climode;
    int m_listenfd;
    unsigned short m_port;
    int m_epollfd;
    epoll_event m_events[MAX_EVENT_NUM];
    HttpConn* m_users;
    Utils m_utils;
};