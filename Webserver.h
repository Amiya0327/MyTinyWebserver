#pragma once
#include<iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "./HttpConn/HttpConn.h"
#include "./Timer/Timer.h"
#include "./ThreadPool/ThreadPool.h"
#include "./ConnPool/ConnPool.h"
#include "./ConnPool/MysqlConn.h"
#include<mutex>
#include "./Logger/Logger.h"

const int MAX_EVENT_NUM = 10000; //最大事件数
const int MAX_FD = 65536; //最大客户连接数
const int MAX_USER_NUM = 10000;
const int TIMESLOT = 3;

class Webserver
{
public:
    Webserver();

    ~Webserver();

    //配置模式初始化
    void init(unsigned short port,int trigmode,bool log_mode,bool log_close,int thread_num,std::string host,unsigned short sqlport,std::string 
    user,std::string passwd,std::string dbname);
    
    void sqlPool(); //数据库连接池初始化

    void threadPool(); //线程池初始化

    void TRIGmode();//触发模式

    //事件循环
    void eventLoop();

    //监听初始化
    void eventListen();

    void Log(const std::string& filename);

private:

    //epoll事件相关
    bool dealWithConn();
    void dealWithRead(int clifd);
    void dealWithWrite(int clifd);
    void dealWithClose(int clifd);
    bool dealWithSignal(bool& timeout, bool& stop);

    //定时器相关
    void Timer(int fd,sockaddr_in addr);
    void addTimer(int fd);
    void delTimer(int fd);
    void closeTimeoutConn(int fd);

    void excute(int fd); //交给线程池处理业务的函数

    std::string m_user;
    std::string m_host;
    std::string m_passwd;
    unsigned short m_sqlport;
    std::string m_dbname;
    bool m_logmode;
    bool m_log_close;
    int m_thread_num;
    int m_lismode;
    int m_climode;
    int m_TRIGmode;
    int m_listenfd;
    unsigned short m_port;
    int m_epollfd;
    int m_pipefd[2];
    epoll_event m_events[MAX_EVENT_NUM];
    HttpConn* m_users;
    Utils m_utils;
};