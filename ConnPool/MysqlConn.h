#pragma once
#include<mysql/mysql.h>
#include<iostream>
#include<chrono>

class MysqlConn
{
public:
    MysqlConn();
    ~MysqlConn();
    bool connect(std::string user,std::string password,std::string dbname,std::string ip,unsigned short port = 3306);
    bool update(std::string sql);
    bool query(std::string sql);
    bool next();
    std::string value(int index);
    unsigned int num_fields();
    bool transaction();
    bool commit();
    bool rollback();
    long long getAlivetime();
    void refreshAlivetime();
private:
    void free_result();
    MYSQL* m_conn = nullptr;
    MYSQL_RES* m_result = nullptr;
    MYSQL_ROW m_row = nullptr;
    std::chrono::steady_clock::time_point m_alivetime;
};

