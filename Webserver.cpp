#include "Webserver.h"


Webserver::Webserver()
{
    m_users = new HttpConn[MAX_FD];
}

Webserver::~Webserver()
{
    close(m_listenfd);
    close(m_epollfd);
    delete[] m_users;
}

void Webserver::init(unsigned short port,int trigmode,int actor_model,std::string host,unsigned short sqlport,std::string 
    user,std::string passwd,std::string dbname)
{
    m_port = port;
    m_TRIGmode = trigmode;
    m_actor_model = actor_model;
    m_sqlport = sqlport;
    m_user = user;
    m_host = host;
    m_passwd = passwd;
    m_sqlport = sqlport;
    m_dbname = dbname;
}

void Webserver::sqlPool()
{
    ConnPool::get_instance().config(m_user,m_dbname,m_passwd,m_host);
    ConnPool::get_instance().reload();
}

void Webserver::eventListen()
{
    m_listenfd = socket(PF_INET,SOCK_STREAM,0);

    if(m_listenfd<0)
    {
        perror("socket");
        exit(-1);
    }

    int opt = 1;
    setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(m_port);
    ser_addr.sin_addr.s_addr = INADDR_ANY;

    //如果port==0，系统bind时自动分配端口由getsockname获取端口号
    if(bind(m_listenfd,(sockaddr*)&ser_addr,sizeof(sockaddr))<0)
    {
        perror("bind");
        exit(-1);
    }

    if(listen(m_listenfd,5)<0)
    {
        perror("listen");
        exit(-1);
    }
    m_epollfd = epoll_create(1);
    if(m_epollfd==-1)
        exit(-1);
    HttpConn::s_epollfd = m_epollfd;
    m_utils.addfd(m_epollfd,m_listenfd,0,m_lismode);
    std::cout << "服务器监听端口" << m_port << std::endl;
    int ret = socketpair(PF_UNIX,SOCK_STREAM,0,m_pipefd);
    if(ret==-1)
    exit(-1);

    m_utils.setnonblocking(m_pipefd[1]);
    m_utils.addfd(m_epollfd,m_pipefd[0],false,0);

    m_utils.addsig(SIGPIPE,SIG_IGN,1);
    m_utils.addsig(SIGALRM,m_utils.sig_handler,1);
    m_utils.addsig(SIGTERM,m_utils.sig_handler,1);

    alarm(TIMESLOT); //信号重入可能导致崩溃
    
    m_utils.init(TIMESLOT);
    m_utils.m_manager.m_closeCallback = bind(&Webserver::closeTimeoutConn,this,std::placeholders::_1);
    Utils::u_epollfd = m_epollfd;
    Utils::u_pipefd = m_pipefd;
}

void Webserver::Timer(int fd,sockaddr_in addr)
{
    m_users[fd].init(fd,addr,m_climode);
    HttpConn::s_user_cnt++;

    addTimer(fd);
}

void Webserver::addTimer(int fd)
{
    time_t cur = time(0);
    Util_timer timer;
    timer.m_expire = cur+3*TIMESLOT;
    timer.m_fd = fd;
    m_utils.m_manager.addTimer(timer);
}

void Webserver::delTimer(int fd)
{
    m_utils.m_manager.delTimer(fd);
}

bool Webserver::dealWithConn()
{
    sockaddr_in client;
    socklen_t len = sizeof(client);
    int clientfd;
    if(m_lismode)
    {
        while(1)
        {
            clientfd = accept(m_listenfd,(sockaddr*)&client,&len);
            if(clientfd<0)
            {
                perror("accept failed");
                return false;
            }
            if (HttpConn::s_user_cnt >= MAX_USER_NUM) 
            {
                // 返回错误信息给客户端
                m_utils.show_error(clientfd,"Server busy");
                //LOG_WARN("Reject connection: too many users");
                return false;
            }
            Timer(clientfd,client);
            std::cout << "客户端连接" << clientfd << std::endl;
        }
    }
    else
    {
        if((clientfd = accept(m_listenfd,(sockaddr*)&client,&len))<0)
        {
            perror("accept failed");
            return false;
        }
        if (HttpConn::s_user_cnt >= MAX_USER_NUM) 
        {
            // 返回错误信息给客户端
            m_utils.show_error(clientfd,"Server busy");
            //LOG_WARN("Reject connection: too many users");
            return false;
        }
        Timer(clientfd,client);

        std::cout << "客户端连接" << clientfd << std::endl;
    }
    return true;
}

void Webserver::eventLoop()
{
    bool stop_server = false;
    bool timeout = false;
    while(!stop_server)
    {
        int event_num = epoll_wait(m_epollfd,m_events,MAX_EVENT_NUM,-1);
        for(int i=0; i<event_num; i++)
        {
            int fd = m_events[i].data.fd;
            if(fd==m_listenfd)
            {
                dealWithConn();
            }
            else if (m_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) //服务器端关闭连接或者发生错误，移除对应的定时器
            {
                dealWithClose(fd);
            }
            else if(m_events[i].data.fd==m_pipefd[0]&& (m_events[i].events&EPOLLIN))
            {
                dealWithSignal(timeout,stop_server);
            }
            else if (m_events[i].events & EPOLLIN) //处理客户连接上接收到的数据
            {
                dealWithRead(fd);
            }
            else if (m_events[i].events & EPOLLOUT)  //处理写事件
            {
                dealWithWrite(fd);
            }
        }
        
        if(timeout)
        {
            m_utils.timer_handler();

            std::cout << "timer tick" << std::endl;

            timeout = 0;
        }
    }
    std::cout << "程序正常退出" << std::endl;
}

void Webserver::TRIGmode()
{
    if(m_TRIGmode==0)
    {
        m_lismode = 0;
        m_climode = 0;
    }
    else if(m_TRIGmode==1)
    {
        m_lismode = 0;
        m_climode = 1;
    }
    else if(m_TRIGmode==2)
    {
        m_lismode = 1;
        m_climode = 0;
    }
    else
    {
        m_lismode = 1;
        m_climode = 1;
    }
}

void Webserver::dealWithRead(int clifd)
{
    addTimer(clifd);
    if(m_actor_model) //reactor
    {
        ThreadPool::get_instance().addTask(std::bind(&Webserver::excute,this,clifd,1));
    }
    else  //proactor
    {
        if(m_users[clifd].read_once())
        {
            ThreadPool::get_instance().addTask(std::bind(&Webserver::excute,this,clifd,1));
        }
        else
        {
            delTimer(clifd);
            m_users[clifd].closeConn();
        }
    }
}

void Webserver::dealWithWrite(int clifd)
{
    if(m_actor_model)
    {
        ThreadPool::get_instance().addTask(std::bind(&Webserver::excute,this,clifd,0));
    }
    else
    {
        if(!m_users[clifd].write())
        {
            delTimer(clifd);
            m_users[clifd].closeConn();
        }
        addTimer(clifd);
    }
}

void Webserver::dealWithClose(int clifd)
{
    delTimer(clifd);
    m_users[clifd].closeConn();
}

void Webserver::excute(int fd, int state)
{
    if(state) //1 读 0 写
    {
        if(m_actor_model) //reactor
        {
            if(m_users[fd].read_once())
            {
                if(!m_users[fd].process())
                {
                    delTimer(fd);
                    m_users[fd].closeConn();
                }
            }
            else
            {
                delTimer(fd);
                m_users[fd].closeConn();
            }
        }
        else  //proactor
        {
            if(!m_users[fd].process())
            {
                delTimer(fd);
                m_users[fd].closeConn();
            }
        }
    }
    else
    {
        if(m_actor_model) //reactor
        {
            if(!m_users[fd].write())
            {
                delTimer(fd);
                m_users[fd].closeConn();
            }
            addTimer(fd);
        }
    }
}

bool Webserver::dealWithSignal(bool& timeout,bool& stop_server)
{
    int ret = 0;
    char signals[1024];

    ret = recv(m_pipefd[0],signals,sizeof(signals),0);
    if(ret<=0)
    return false;
    else
    {
        for(int i=0; i<ret; i++)
        {
            if(signals[i]==SIGALRM)
            {
                timeout = true;
            }
            else if(signals[i]==SIGTERM)
            {
                stop_server = true;
            }
        }
    }

    return true;
}

void Webserver::closeTimeoutConn(int fd)
{
    delTimer(fd);
    m_users[fd].closeConn();
}
