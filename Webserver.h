#pragma once
#include<iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "HttpConn.h"
#include "Timer.h"
#include "./ThreadPool/ThreadPool.h"

const int MAX_EVENT_NUM = 10000; //最大事件数
const int MAX_FD = 65536; //最大客户连接数
const int MAX_USER_NUM = 10000;
const int TIMESLOT = 5;

class Webserver
{
public:
    Webserver();

    ~Webserver();

    //配置模式初始化
    void init(unsigned short port,int trigmode,int actor_mode);

    void TRIGmode();

    //事件循环
    void eventLoop();

    //监听初始化
    void eventListen();

private:

    bool dealWithConn();
    void dealWithRead(int clifd);
    void dealWithWrite(int clifd);
    void dealWithClose(int clifd);
    bool dealWithSignal(bool& timeout, bool& stop);
    void excute(int fd,int state); //交给线程池处理业务的函数

    int m_lismode;
    int m_climode;
    int m_TRIGmode;
    int m_actor_model;
    int m_listenfd;
    unsigned short m_port;
    int m_epollfd;
    int m_pipefd[2];
    epoll_event m_events[MAX_EVENT_NUM];
    HttpConn* m_users;
    Utils m_utils;
};