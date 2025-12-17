#pragma once 
#include <fcntl.h>
#include<sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include<string.h>
#include<iostream>
#include<queue>
#include <signal.h>
#include <arpa/inet.h>
#include<unordered_map>
#include<functional>


//定时器节点
class Util_timer
{
public:
    Util_timer();
    ~Util_timer();
    int m_fd;
    time_t m_expire;
};

class TimerCompare
{
public:
    bool operator()(Util_timer& t1, Util_timer& t2);
};

class TimerManager
{
public:
    TimerManager();
    ~TimerManager();
    void addTimer(Util_timer); //调整定时器
    void delTimer(int ); //删除定时器
    void tick(); //闹钟触发处理
    std::function<void(int)> m_closeCallback;
private:
    std::priority_queue<Util_timer,std::vector<Util_timer>,TimerCompare> m_timer_pq; //优先队列存储定时器
    std::unordered_map<int,time_t> m_cur_expire; //每个连接的最新超时时间
};

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

    //移除fd
    void removefd(int epollfd,int fd);

    void modfd(int epollfd, int fd, int ev,bool TRIGmode); //重设oneshot

    //信号函数修改
    void addsig(int signal,void handle(int),bool restart);

    //处理超时定时器
    void timer_handler();

    void init(int timeslot); 

    //闹钟触发调用函数
    static void sig_handler(int sig);
    
    static int* u_pipefd; //socketpair创建的fd
    static int u_epollfd;
    TimerManager m_manager;

private:
    int m_timeslot;
};