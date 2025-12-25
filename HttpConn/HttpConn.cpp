#include "HttpConn.h"

int HttpConn::s_epollfd = -1;
std::atomic<int> HttpConn::s_user_cnt = 0;

HttpConn::HttpConn()
{

}

HttpConn::~HttpConn()
{

}

int HttpConn::fd()
{
    return m_clifd;
}
sockaddr_in HttpConn::addr()
{
    return m_addr;
}

void setnonblocking(int fd)
{
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);
}

//注册epoll事件
void addfd(int epollfd, int fd,bool one_shot,bool TRIGmode)
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

//从epoll上移除
void removefd(int epollfd,int fd)
{
    if(fd==-1)
    return;
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
    //std::cout << "客户端连接断开" << std::endl;
}

//修改epoll状态，每次重新设置oneshot
void modfd(int epollfd, int fd, int event,bool TRIGmode)
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

void HttpConn::init(int fd,const sockaddr_in& addr,int trigmode)
{
    m_clifd = fd;
    m_addr = addr;
    m_TRIGmode = trigmode;
    addfd(s_epollfd,m_clifd,0,m_TRIGmode);

    init();
}

void HttpConn::init()
{
    m_read_idx = 0;
    m_write_idx = 0;
    m_checked_idx = 0;
    m_start_line = 0;
    m_TRIGmode = 0;
    m_check_state = CHECK_STATE::CHECK_STATE_REQUESTLINE;
    m_url = 0;
    METHOD m_method = GET;
    m_version =  0;
    cgi = 0;  //判断是否启用post
    m_host = 0;
    m_content_length = 0;
    m_linger = false;
    m_file_addr = 0;
    m_iv_cnt = 0;
    m_bytes_to_send = 0;
    m_host = 0;
    m_bytes_have_send = 0;
    m_string = 0;
    memset(m_real_file,0,sizeof(m_real_file));
    memset(m_read_buf,0,sizeof(m_read_buf));
    memset(m_write_buf,0,sizeof(m_write_buf));
}

void HttpConn::closeConn()
{
    if(m_clifd==-1)
    return;
    removefd(s_epollfd,m_clifd);
    m_clifd = -1;
    s_user_cnt--;
    LOG_INFO("connection of client(%s) closed",inet_ntoa(m_addr.sin_addr));
}


bool HttpConn::read_once()
{
    if (m_read_idx >= READ_BUFFER_SIZE-1)
    {
        return false;
    }
    int bytes_read = 0;

    //ET读取数据
    if (m_TRIGmode)
    {
        while(1)
        {
            bytes_read = recv(m_clifd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx-1, 0);
            if(bytes_read==-1)
            {
                if(errno==EAGAIN||errno==EWOULDBLOCK)
                {
                    break;
                }
                return false;
            }
            else if(bytes_read==0)
            {
                return false;
            }
            m_read_idx += bytes_read;
        }
    }
    else //LT读数据
    {
        bytes_read = recv(m_clifd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx-1, 0);
        if(bytes_read==-1)
        {
            if(errno==EAGAIN||errno==EWOULDBLOCK)
            {
                return true;
            }
            return false;
        }
        else if(bytes_read==0)
        {
            return false;
        }
        m_read_idx += bytes_read;
    }

    return true;
}

bool HttpConn::process()
{
    HTTP_CODE read_ret = process_read();
    if(read_ret == HTTP_CODE::NO_REQUEST) //请求不完整，等待重新读取
    {
        modfd(s_epollfd,m_clifd,EPOLLIN,m_TRIGmode);
        return true;
    }
    bool write_ret = process_write(read_ret);
    if(!write_ret) //写入缓冲区失败，断开连接
    {
        return false;
    }
    modfd(s_epollfd,m_clifd,EPOLLOUT,m_TRIGmode); //注册读事件，等待发送响应报文
    return true;
}

char* HttpConn::get_line()
{
    return m_read_buf+m_start_line;
}

HttpConn::HTTP_CODE HttpConn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;
    //让读取内容则跳过解析，直接读取，否则解析一行
    while( (m_check_state==CHECK_STATE_CONTENT&&line_status==LINE_OK) || ((line_status = parse_line()) == LINE_OK) )
    {
        text = get_line();
        LOG_INFO("%s",text);
        // std::cout << text<< std::endl;
        m_start_line = m_checked_idx;

        //每次解析完成更新m_check_state
        if(m_check_state==CHECK_STATE_REQUESTLINE)
        {
            ret = parse_request_line(text);
            if(ret==BAD_REQUEST)
                return BAD_REQUEST;
        }
        else if(m_check_state==CHECK_STATE_HEADER)
        {
            ret = parse_headers(text);
            if(ret==BAD_REQUEST)
                return BAD_REQUEST;
            else if(ret==GET_REQUEST)
                return do_request();
        }
        else if(m_check_state==CHECK_STATE_CONTENT)
        {
            ret = parse_content(text);
            if(ret==GET_REQUEST)
                return do_request();
            line_status = LINE_OPEN; //内容不完整，设置LINE_OPEN
        }
        else
        {
            return INTERNAL_ERROR; //内部错误
        }
    }

    return NO_REQUEST;
}

bool HttpConn::add_response(const char *format, ...)
{
    if(m_write_idx>=WRITE_BUFFER_SIZE)
    {
        return false;
    }
    va_list args;
    va_start(args,format);
    int len = vsnprintf(m_write_buf+m_write_idx,WRITE_BUFFER_SIZE-1-m_write_idx,format,args);
    if(m_write_idx+len>=WRITE_BUFFER_SIZE)
    {
        va_end(args);
        return false;
    }
    m_write_idx+=len;
    va_end(args);

    //打印日志
    LOG_INFO("response:%s",m_write_buf);
    return true;
}

bool HttpConn::add_content(const char *content)
{
    return add_response(content);
}

bool HttpConn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n","HTTP/1.1",status,title);
}

bool HttpConn::add_headers(int content_length)
{
    return add_content_length(content_length)&&add_linger()&&add_blank_line();
}

bool HttpConn::add_content_type()
{
    return add_response("Content-Type: %s\r\n","text/html"); //暂时只有静态网页html
}

bool HttpConn::add_content_length(int content_length)
{
    return add_response("Content-Length:%d\r\n",content_length);
}

bool HttpConn::add_linger()
{
    return add_response("Connecion:%s\r\n",m_linger?"keep-alive":"close");
}

bool HttpConn::add_blank_line()
{
    return add_response("\r\n");
}


bool HttpConn::process_write(HTTP_CODE read_ret)
{

    //根据解析情况生成响应报文
    if(read_ret==HTTP_CODE::BAD_REQUEST||read_ret==HttpConn::NO_RESOURCE)
    {
        add_status_line(404,error_404_title);
        add_headers(strlen(error_404_form));
        if(!add_content(error_404_form))
            return false;
    }
    else if(read_ret==HTTP_CODE::FILE_REQUEST)
    {
        add_status_line(200,ok_200_title);
        if(m_file_stat.st_size)
        {
            add_headers(m_file_stat.st_size);
            m_iv[0].iov_base = m_write_buf;
            m_iv[0].iov_len = m_write_idx;
            m_iv[1].iov_base = m_file_addr;
            m_iv[1].iov_len = m_file_stat.st_size;
            m_iv_cnt = 2;
            m_bytes_to_send = m_write_idx+m_file_stat.st_size;
            //std::cout << "文件写入缓冲区成功" << std::endl;
            return true;
        }
        else
        {
            const char* ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if(!add_content(ok_string))
                return false;
        }

    }
    else if(read_ret==HTTP_CODE::FORBIDDEN_REQUEST)
    {
        add_status_line(403,error_403_title);
        add_headers(strlen(error_403_form));
        if(!add_content(error_403_form))
            return false;
    }
    else if(read_ret == HTTP_CODE::INTERNAL_ERROR)
    {
        add_status_line(500,error_500_title);
        add_headers(strlen(error_500_form));
        if(!add_content(error_500_form))
            return false;
    }
    else
    {
        return false;
    }

    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_cnt = 1;
    m_bytes_to_send = m_write_idx;
    return true;
}

HttpConn::LINE_STATUS HttpConn::parse_line()
{
    char temp;
    //检查一行的格式
    for(; m_checked_idx<m_read_idx; m_checked_idx++)
    {
        temp = m_read_buf[m_checked_idx];
        if(temp=='\r')
        {
            //数据不完整
            if(m_checked_idx+1==m_read_idx)
                return LINE_OPEN;
            else if(m_read_buf[m_checked_idx+1]=='\n') //'\r'后面必须'\n'，否则出错
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            else
                return LINE_BAD;
        }
        else if(temp=='\n')
        {
            if(m_checked_idx>0&&m_read_buf[m_checked_idx-1]=='\r')
            {
                m_read_buf[m_checked_idx-1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            else
            return LINE_BAD;
        }
    }

    return LINE_OPEN;
}

HttpConn::HTTP_CODE HttpConn::parse_request_line(char* text)
{
    //std::cout << "解析请求行" << std::endl;
    //GET / HTTP/1.1
    m_url = strpbrk(text," \t"); //跳到第一个空空白字符，此时指向method的后一个字符
    if(!m_url)
    return BAD_REQUEST;

    //取出方法
    *m_url = '\0';  
    m_url++;
    char* method = text;

    if(strcasecmp(method,"GET")==0)
    {
        m_method = GET;
    }
    else if(strcasecmp(method,"POST")==0)
    {
        m_method = POST;
        cgi = 1;
    }
    else
    return BAD_REQUEST;

    m_url += strspn(m_url," \t");  //跳过空白字符，此时指向第一个'/'位置
    m_version = strpbrk(m_url," \t");
    if(!m_version)
        return BAD_REQUEST;

    //取出url
    *m_version = '\0';
    m_version++;

    m_version += strspn(m_version," \t"); //跳过空白字符，指向版本信息的第一个字符
    if(strcasecmp(m_version,"HTTP/1.1")==0)
        m_linger = 1;
    else if(strcasecmp(m_version,"HTTP/1.0")==0)
        m_linger = 0;
    else
        return BAD_REQUEST;

    //跳过多余信息
    if(strncasecmp(m_url,"http://",7)==0)
    {
        m_url+=7;
        m_url = strchr(m_url,'/');
    }
    if(strncasecmp(m_url,"https://",8)==0)
    {
        m_url+=8;
        m_url = strchr(m_url,'/');
    }

    if(!m_url||m_url[0]!='/')
        return BAD_REQUEST;
    
    if(m_url[strlen(m_url)-1]=='/')
    {
        strcat(m_url,"index.html");
    }
    
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::parse_headers(char *text)
{
    //std::cout << "解析请求头" << std::endl;

    if(text[0]=='\0') //空行
    {
        if(m_content_length!=0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if(strncasecmp(text,"Connection:",11)==0)
    {
        text+=11;
        text+= strspn(text," \t");
        if(strcasecmp(text,"keep-alive")==0) //持久连接
        {
            m_linger = true;
        }
        else 
        {
            m_linger = false;
        }
    }
    else if(strncasecmp(text,"Content-length:",15)==0)
    {
        text+=15;
        text+= strspn(text," \t");
        m_content_length = atoll(text);
    }
    else if(strncasecmp(text,"Host:",5)==0)
    {
        text+=5;
        text+=strspn(text," \t");
        m_host = text;
    }
    else
    {
        LOG_INFO("unknown header:%s",text);
        //错误头，打印日志
    }
    return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::parse_content(char *text)
{
    //std::cout << "解析内容" << std::endl;
    if(m_read_idx>=m_content_length+m_check_state)
    {
        text[m_content_length] = '\0';
        m_string = text;  //保存内容
        return GET_REQUEST;
    }
    
    return NO_REQUEST;
}

//解析请求和路径，确认返回浏览器的状态
HttpConn::HTTP_CODE HttpConn::do_request()
{
    strcpy(m_real_file,doc_root);
    int len = strlen(doc_root);
    //printf("m_url:%s\n", m_url);
    const char *p = strrchr(m_url, '/'); //跳转到最后一个'/'
    std::string url(p);
    if(cgi==1)  //post请求，cqi处理
    {
        std::string user;
        std::string passwd;
        bool exit_flag;
        //user=yzl&password=123  内容格式
        if(m_string)
        {
            //取出用户名和密码
            int i;
            for(i=5; m_string[i]!='&'; i++)
            {
                user+=m_string[i];
            }
            for(i+=10; m_string[i]; i++)
            {
                passwd+=m_string[i];
            }

            //取出数据库连接，查询用户是否存在
            //std::cout << "用户:" << user << " 密码:" << passwd << std::endl;
            std::string query = "select id from user where username='" + user + "' && password='"+passwd +"';";
            std::shared_ptr<MysqlConn> sql_conn = ConnPool::get_instance().getConnection();
            sql_conn->query(query);
            exit_flag = sql_conn->next();
            //std::cout << "用户存在:" << exit_flag << std::endl;
        }
        if(url=="/CGISQL_LOG.cgi")  //登录请求
        {
            if(exit_flag) //用户存在正常登录，否则返回登录错误信息
            {
                strncpy(m_real_file + len, "/welcome.html", FILENAME_LEN - len - 1);
            }
            else
            {
                strncpy(m_real_file + len, "/logError.html", FILENAME_LEN - len - 1);
            }
        }
        else if(url=="/CGISQL_REGISTER.cgi") //注册请求
        {
            if(exit_flag)
            {
                strncpy(m_real_file + len, "/registerError.html", FILENAME_LEN - len - 1);
            }
            else
            {
                //注册成功往数据库插入数据
                std::string insert = "insert into user (username,password) values('" + user + "','" + passwd +"');";
                ConnPool::get_instance().getConnection()->update(insert);
                strncpy(m_real_file + len, "/log.html", FILENAME_LEN - len - 1);
            }
        }
        else
        return BAD_REQUEST;
    }
    else
    {
        if(url.back()=='?') //处理get表单，去掉行尾'?'
        url.pop_back();
        if(url=="/register")  //注册页面
        {
            strncpy(m_real_file + len, "/register.html", FILENAME_LEN - len - 1);

        }
        else if(url=="/log") //登录页面
        {
            strncpy(m_real_file + len, "/log.html", FILENAME_LEN - len - 1);
        }
        else if(url=="/picture") //请求图片
        {
            strncpy(m_real_file + len, "/picture.html", FILENAME_LEN - len - 1);
        }
        else if(url=="/video") //请求视频
        {
            strncpy(m_real_file + len, "/video.html", FILENAME_LEN - len - 1);
        }
        else if(url=="/fans") //请求关注页面
        {
            strncpy(m_real_file + len, "/fans.html", FILENAME_LEN - len - 1);
        }
        else
        {
            strncpy(m_real_file + len, url.c_str(), FILENAME_LEN - len - 1);
        }
    }

    if (stat(m_real_file, &m_file_stat) < 0)//文件不存在
        return NO_RESOURCE;

    if(!(m_file_stat.st_mode&S_IROTH)) //权限不够
        return FORBIDDEN_REQUEST;

    if(S_ISDIR(m_file_stat.st_mode)) //错误访问文件夹
        return BAD_REQUEST;

    int fd = open(m_real_file,O_RDONLY);
    m_file_addr = (char*)mmap(0,m_file_stat.st_size,PROT_READ,MAP_PRIVATE,fd,0); //文件映射
    close(fd);
    //std::cout << "m_url:" << m_real_file << std::endl;
    return FILE_REQUEST;
}

bool HttpConn::write()
{
    int temp = 0;

    if(m_bytes_to_send==0)  //数据不完整，重新初始化
    {
        modfd(s_epollfd,m_clifd,EPOLLIN,m_TRIGmode);
        init();
        return true;
    }

    while(1)
    {
        temp = writev(m_clifd,m_iv,m_iv_cnt);

        if(temp<0)
        {
            if(errno==EAGAIN) //暂时无法发送,等待
            {
                modfd(s_epollfd,m_clifd,EPOLLOUT,m_TRIGmode);
                return true;
            }
            unmap();  //其他错误，关闭map映射并关闭连接
            return false;
        }
        
        //向量批量写入
        m_bytes_have_send+=temp;
        m_bytes_to_send-=temp;
        if(m_bytes_have_send>=m_write_idx)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_addr + (m_bytes_have_send-m_write_idx);
            m_iv[1].iov_len = m_bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf+m_bytes_have_send;
            m_iv[0].iov_len = m_write_idx-m_bytes_have_send;
        }

        if(m_bytes_to_send<=0)
        {
            //std::cout << "文件发送成功:共" << m_bytes_have_send <<"个字节" << std::endl;
            unmap();  //发送完成，关闭文件映射
            if(m_linger)  //持久连接
            {
                modfd(s_epollfd,m_clifd,EPOLLIN,m_TRIGmode);
                init();   //初始化，不关闭连接
                //std::cout << "初始化完成" << std::endl;
                return true; 
            }
            else
                return false;  //关闭连接
        }

    }
}

void HttpConn::unmap()
{
    if(m_file_addr)
    {
        munmap(m_file_addr,m_file_stat.st_size);
        m_file_addr = 0;
    }
}