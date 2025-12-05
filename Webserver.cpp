#include "Webserver.h"


Webserver::Webserver()
{
    m_users = new HttpConn[MAX_FD];
}

Webserver::~Webserver()
{
    close(m_listenfd);
    delete[] m_users;
}

void Webserver::init(unsigned short port,int trigmode,int actor_model)
{
    m_port = port;
    m_TRIGmode = trigmode;
    m_actor_model = actor_model;
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
    HttpConn::s_epollfd = m_epollfd;
    m_utils.addfd(m_epollfd,m_listenfd,0,m_lismode);
    std::cout << "服务器监听端口" << m_port << std::endl;
}

void Webserver::dealWithConn()
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
                return;
            }
            if (HttpConn::s_user_cnt >= MAX_USER_NUM) 
            {
                // 返回错误信息给客户端
                m_utils.show_error(clientfd,"Server busy");
                //LOG_WARN("Reject connection: too many users");
                return;
            }
            m_users[clientfd].init(clientfd,client,m_climode);
            m_utils.addfd(m_epollfd,clientfd,0,m_lismode);
            HttpConn::s_user_cnt++;
            std::cout << "客户端连接" << clientfd << std::endl;
        }
    }
    else
    {
        if((clientfd = accept(m_listenfd,(sockaddr*)&client,&len))<0)
        {
            perror("accept failed");
            return;
        }
        if (HttpConn::s_user_cnt >= MAX_USER_NUM) 
        {
            // 返回错误信息给客户端
            m_utils.show_error(clientfd,"Server busy");
            //LOG_WARN("Reject connection: too many users");
            return;
        }
        m_users[clientfd].init(clientfd,client,m_climode);
        m_utils.addfd(m_epollfd,clientfd,0,m_lismode);
        HttpConn::s_user_cnt++;
        std::cout << "客户端连接" << clientfd << std::endl;
    }

}

void Webserver::eventLoop()
{
    bool stop_server = false;
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
            else if (m_events[i].events & EPOLLIN) //处理客户连接上接收到的数据
            {
                dealWithRead(fd);
            }
            else if (m_events[i].events & EPOLLOUT)  //处理写事件
            {
                dealWithWrite(fd);
            }
            //处理信号待补充
        }
    }
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
    if(m_actor_model) //reactor
    {
        ThreadPool::get_instance().addTask(bind(&Webserver::excute,this,clifd,1));
    }
    else  //proactor
    {
        if(m_users[clifd].read_once())
        {
            ThreadPool::get_instance().addTask(bind(&Webserver::excute,this,clifd,1));
        }
        else
        {
            m_users[clifd].closeConn();
        }
    }
}

void Webserver::dealWithWrite(int clifd)
{
    if(m_actor_model)
    {
        ThreadPool::get_instance().addTask(bind(&Webserver::excute,this,clifd,0));
    }
    else
    {
        if(!m_users[clifd].write())
            m_users[clifd].closeConn();
    }
}

void Webserver::dealWithClose(int clifd)
{
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
                m_users[fd].process();
            }
            else
            {
                m_users[fd].closeConn();
            }
        }
        else  //proactor
        {
            m_users[fd].process();
        }
    }
    else
    {
        if(m_actor_model) //reactor
        {
            if(!m_users[fd].write())
                m_users[fd].closeConn();
        }
    }
}