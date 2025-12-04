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
#include "Timer.h"
#include <sys/uio.h>

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
    bool read_once();
    void process();
    bool write();
    void unmap();

    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line();
    LINE_STATUS parse_line();

    void closeConn();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();    

    static int s_epollfd;
    static int s_user_cnt;
    static Utils s_utils;
private:
    char m_read_buf[READ_BUFFER_SIZE];
    char m_write_buf[WRITE_BUFFER_SIZE];
    struct iovec m_iv[2];
    int m_iv_cnt;
    int m_bytes_to_send;
    int m_bytes_have_send;
    int m_clifd;
    int m_read_idx;
    int m_write_idx;
    int m_checked_idx;
    int m_start_line;
    int m_TRIGmode;
    char* m_url;
    char* m_host;
    long long m_content_length;
    bool m_linger;  //保活连接
    char* m_string; //存储请求头数据
    char m_real_file[FILENAME_LEN];
    struct stat m_file_stat;
    char* m_file_addr;

    METHOD m_method;
    char* m_version;
    int cgi;  //判断是否启用post
    CHECK_STATE m_check_state;
    sockaddr_in m_addr;
};