#pragma once
#include<iostream>
#include<queue>
#include<mutex>
#include<string>
#include<condition_variable>
#include "MysqlConn.h"
#include<thread>

class ConnPool
{
public:
    //单例模式
    ConnPool(const ConnPool& cp) = delete;

    ConnPool& operator=(const ConnPool& cp) = delete;

    ~ConnPool();

    static ConnPool& get_instance();

    void config(std::string user,std::string dbName,std::string password,std::string ip,int min = 15,int max = 30,unsigned short port = 3306);

    void reload();

    std::shared_ptr<MysqlConn> getConnection();

private:

    void produceConnection();

    void recycleConnection();

    void addConnection();

    ConnPool();

    std::string m_user;
    std::string m_dbName;
    std::string m_password;
    std::string m_ip;
    unsigned short m_port;
    int m_max;
    int m_min;
    int m_timeout;
    int m_MaxIdleTime;
    bool m_isConfig;
    bool m_stop;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    std::queue<MysqlConn*> m_connectionQ;
};
