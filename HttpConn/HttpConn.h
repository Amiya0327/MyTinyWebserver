#pragma once
#include <arpa/inet.h>
#include<string.h>
#include<string>
#include<algorithm>
#include<fstream>
#include<iostream>
#include<sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include<unistd.h>
#include <stdarg.h>
#include<sys/epoll.h>
#include <sys/uio.h>
#include <atomic>
#include "../ConnPool/ConnPool.h"
#include "../Logger/Logger.h"

const int READ_BUFFER_SIZE = 2048;
const int WRITE_BUFFER_SIZE = 1024;
const int FILENAME_LEN = 200;

inline const char* doc_root = "./root";

inline const char *ok_200_title = "OK";
inline const char *error_400_title = "Bad Request";
inline const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
inline const char *error_403_title = "Forbidden";
inline const char *error_403_form = "You do not have permission to get file form this server.\n";
inline const char *error_404_title = "Not Found";
inline const char *error_404_form = "The requested file was not found on this server.\n";
inline const char *error_500_title = "Internal Error";
inline const char *error_500_form = "There was an unusual problem serving the request file.\n";

class HttpConn
{
public:
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    HttpConn();
    ~HttpConn();
    //accept时初始化
    void init(int fd,const sockaddr_in& addr,int trigmode);
    //请求完成时初始化缓冲区等数据
    void init();
    bool read_once(); //读取缓冲区数据
    bool process(); //解析http
    bool write(); //发送响应报文
    void unmap(); //关闭文件映射

    HTTP_CODE process_read(); //解析读缓冲区数据
    bool process_write(HTTP_CODE ret); //生成报文到读缓冲区
    HTTP_CODE parse_request_line(char *text); //解析请求行
    HTTP_CODE parse_headers(char *text); //解析头
    HTTP_CODE parse_content(char *text); //解析内容
    HTTP_CODE do_request(); //根据请求预处理响应报文
    LINE_STATUS parse_line(); //解析一行数据
    char *get_line(); //从读缓冲区读取一行数据

    void closeConn();
    bool add_response(const char *format, ...);  //统一往写缓冲区添加信息
    bool add_content(const char *content); //添加内容
    bool add_status_line(int status, const char *title); //添加响应状态
    bool add_headers(int content_length); //添加响应头
    bool add_content_type(); //添加内容类型
    bool add_content_length(int content_length); //内容长度
    bool add_linger(); //是否持久连接
    bool add_blank_line(); //结束的空行
    int fd(); 
    sockaddr_in addr();

    static int s_epollfd;
    static std::atomic<int> s_user_cnt; //多线程下访问，需要原子变量，
private:
    char m_read_buf[READ_BUFFER_SIZE]; //读缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE]; //写缓冲区

    struct iovec m_iv[2]; //读写向量，用于批量发送数据，优化性能
    int m_iv_cnt;

    int m_bytes_to_send;
    int m_bytes_have_send;
    int m_clifd;
    sockaddr_in m_addr;
    int m_read_idx; //读缓冲区内容大小
    int m_write_idx; //写缓冲区内容大小
    int m_checked_idx; //已经解析内容，当前解析位置
    int m_start_line; //读取一行的开始位置
    int m_TRIGmode; //触发模式
    char* m_url; //资源
    char* m_host;
    long long m_content_length;
    bool m_linger;  //保活连接
    char* m_string; //存储请求头数据
    char m_real_file[FILENAME_LEN]; //真实文件位置
    struct stat m_file_stat; //文件状态
    char* m_file_addr;

    //解析状态
    METHOD m_method;
    char* m_version;  //http版本
    int cgi;  //判断是否启用post
    CHECK_STATE m_check_state; 
};