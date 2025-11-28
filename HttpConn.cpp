#include "HttpConn.h"

int HttpConn::s_epollfd = -1;
int HttpConn::s_user_cnt = 0;

HttpConn::HttpConn()
{

}

HttpConn::~HttpConn()
{

}

void HttpConn::init(int fd,const sockaddr_in& addr)
{
    m_clifd = fd;
    m_addr = addr;
}