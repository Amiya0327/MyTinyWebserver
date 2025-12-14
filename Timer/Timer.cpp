#include "Timer.h"

int* Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

bool TimerCompare::operator()(Util_timer& t1, Util_timer& t2)
{
    return t1.m_expire > t2.m_expire;
}

Util_timer::Util_timer()
{
    
}

Util_timer::~Util_timer()
{

}

Utils::Utils()
{

}

Utils::~Utils()
{

}

TimerManager::TimerManager()
{

}

TimerManager::~TimerManager()
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

void Utils::removefd(int epollfd,int fd)
{
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
    //std::cout << "客户端连接断开" << std::endl;
}

void Utils::modfd(int epollfd, int fd, int event,bool TRIGmode)
{
    epoll_event ev;
    ev.data.fd = fd;
    if(TRIGmode)
        ev.events = event|EPOLLONESHOT|EPOLLET|EPOLLRDHUP;
    else
        ev.events = event|EPOLLONESHOT|EPOLLRDHUP;

    setnonblocking(fd);
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);   
}

void Utils::addsig(int signal,void handle(int),bool restart)
{
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = handle;
    if(restart)
        sa.sa_flags|= SA_RESTART;
    sigfillset(&sa.sa_mask);
    if(sigaction(signal,&sa,0)==-1)
        exit(-1);
}

void Utils::sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

void Utils::timer_handler()
{
    m_manager.tick();
    alarm(m_timeslot);
}

void Utils::init(int timeslot) 
{
    m_timeslot = timeslot;
}

void TimerManager::addTimer(Util_timer timer)
{
    m_cur_expire[timer.m_fd] = timer.m_expire;
    m_timer_pq.emplace(timer);
}

void TimerManager::delTimer(int fd)
{
    m_cur_expire.erase(fd);
}

void TimerManager::tick()
{
    time_t cur = time(0);

    while(!m_timer_pq.empty())
    {
        Util_timer timer = m_timer_pq.top();
        if(m_cur_expire.find(timer.m_fd)==m_cur_expire.end())
        {
            m_timer_pq.pop();
            continue;
        }
        if(timer.m_expire<cur)
        {
            m_timer_pq.pop();
            if(m_cur_expire[timer.m_fd]==timer.m_expire)
            {
                m_closeCallback(timer.m_fd);
                //std::cout << "连接超时,关闭fd:"<< timer.m_fd <<  "和定时器" << std::endl;
            }
        }
        else
            break;
    }
}
