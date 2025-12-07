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
    void addTimer(Util_timer);
    void delTimer(int );
    void tick();
    std::function<void(int)> m_closeCallback;
private:
    std::priority_queue<Util_timer,std::vector<Util_timer>,TimerCompare> m_timer_pq;
    std::unordered_map<int,time_t> m_cur_expire;
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

    void removefd(int epollfd,int fd);

    void modfd(int epollfd, int fd, int ev,bool TRIGmode); //重设oneshot

    void addsig(int signal,void handle(int),bool restart);

    void timer_handler();

    void init(int timeslot); 

    static void sig_handler(int sig);
    
    static int* u_pipefd;
    static int u_epollfd;
    TimerManager m_manager;

private:
    int m_timeslot;
};