#include "MysqlConn.h"

MysqlConn::MysqlConn()
{
    m_conn = mysql_init(nullptr);
    mysql_set_character_set(m_conn,"utf8");
}

MysqlConn::~MysqlConn()
{
    if(m_conn)
    mysql_close(m_conn);
    free_result();
}

bool MysqlConn::connect(std::string user, std::string password, std::string dbname, std::string ip, unsigned short port)
{
    MYSQL* ptr = mysql_real_connect(m_conn,ip.c_str(),user.c_str(),password.c_str(),dbname.c_str(),port,nullptr,0);
    return ptr!=nullptr;
}

bool MysqlConn::update(std::string sql)
{
    if(mysql_query(m_conn,sql.c_str()))
    return false;
    else
    return true;
}

bool MysqlConn::query(std::string sql)
{
    free_result();
    if(mysql_query(m_conn,sql.c_str()))
    return false;
    m_result = mysql_store_result(m_conn);
    return true;
}

bool MysqlConn::next()
{
    if(m_result!=nullptr)
    {
        m_row = mysql_fetch_row(m_result);
        if(m_row)
        return true;
        else
        return false;
    }
    return false;
}

std::string MysqlConn::value(int index)
{
    if(m_result==nullptr||m_row==nullptr)
    return std::string();
    unsigned int colNum = mysql_num_fields(m_result);
    if(index>=colNum||index<0)
    return std::string();
    char* val = m_row[index];
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    return std::string(val,length);
}

unsigned int MysqlConn::num_fields()
{
    if(m_result==nullptr)
    return 0;

    return mysql_num_fields(m_result);
}

bool MysqlConn::transaction()
{
    return mysql_autocommit(m_conn,false);
}

bool MysqlConn::commit()
{
    return mysql_commit(m_conn);
}

bool MysqlConn::rollback()
{
    return mysql_rollback(m_conn);
}

void MysqlConn::free_result()
{
    if(m_result)
    {
        mysql_free_result(m_result);
        m_result = nullptr;
    }
}

long long MysqlConn::getAlivetime()
{
    auto interval = std::chrono::steady_clock::now()-m_alivetime;
    std::chrono::milliseconds ret = std::chrono::duration_cast<std::chrono::milliseconds>(interval);
    return ret.count();
}

void MysqlConn::refreshAlivetime()
{
    m_alivetime = std::chrono::steady_clock::now();
}