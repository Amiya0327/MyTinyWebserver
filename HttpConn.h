#pragma once
#include <arpa/inet.h>


class HttpConn
{
public:
    HttpConn();
    ~HttpConn();
    void init(int fd,const sockaddr_in& addr);
    static int s_epollfd;
    static int s_user_cnt;
private:
    int m_clifd;
    sockaddr_in m_addr;
};