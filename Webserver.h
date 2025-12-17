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
const int MAX_USER_NUM = 10000; //最大用户数量
const int TIMESLOT = 3; //最小定时器触发时间间隔

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

    //日志初始化
    void Log(const std::string& filename);

private:

    //epoll事件相关
    bool dealWithConn(); //处理连接请求
    void dealWithRead(int clifd); //处理读事件
    void dealWithWrite(int clifd); //处理写事件
    void dealWithClose(int clifd); //处理断连
    bool dealWithSignal(bool& timeout, bool& stop); //处理信号

    //定时器相关
    void Timer(int fd,sockaddr_in addr); //初始化定时器
    void addTimer(int fd); //调整定时器
    void delTimer(int fd); //删除定时器
    void closeTimeoutConn(int fd); //超时处理

    void excute(int fd); //交给线程池处理业务的函数

    std::string m_user; //数据库用户名
    std::string m_host; //数据库主机名
    std::string m_passwd; //数据库密码
    unsigned short m_sqlport; //数据库端口
    std::string m_dbname; //数据库名
    bool m_logmode; //日志模式
    bool m_log_close; //日志关闭标志
    int m_thread_num; //线程池线程数量
    int m_lismode; //监听模式
    int m_climode; //读写模式
    int m_TRIGmode;
    int m_listenfd;
    unsigned short m_port;
    int m_epollfd;
    int m_pipefd[2]; //socketpair创建的fd
    epoll_event m_events[MAX_EVENT_NUM];  
    HttpConn* m_users; //用户数组
    Utils m_utils; //定时器管理
};