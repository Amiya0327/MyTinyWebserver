#include "ConnPool.h"

ConnPool::ConnPool()
{
    m_isConfig = false;
    m_stop = false;
}

ConnPool& ConnPool::get_instance()
{
    static ConnPool cp;
    return cp;
}

ConnPool::~ConnPool()
{
    m_stop = true;
    m_cv.notify_all();
    while(!m_connectionQ.empty())
    {
        delete m_connectionQ.front();
        m_connectionQ.pop();
    }
}

void ConnPool::config(std::string user,std::string dbName,std::string password,std::string ip,int min,int max,unsigned short port)
{
    m_isConfig = true;
    m_user = user;
    m_dbName = dbName;
    m_password = password;
    m_ip = ip;
    m_min = min;
    m_max = max;
    m_port = port;
    m_MaxIdleTime = 5000;
    m_timeout = 1000;
}

void ConnPool::reload()
{
    if(!m_isConfig)
    return;
    for(int i=0; i<m_min; i++)
    {
        addConnection();
    }
    std::thread producer(&ConnPool::produceConnection,this);
    std::thread recycler(&ConnPool::recycleConnection,this);

    producer.detach();
    recycler.detach();
}

void ConnPool::addConnection()
{
    MysqlConn* mc = new MysqlConn;
    mc->connect(m_user,m_password,m_dbName,m_ip,m_port);
    m_connectionQ.emplace(mc);
    mc->refreshAlivetime();
}

void ConnPool::produceConnection()
{
    while(1)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cv.wait(lock,[this](){return m_stop||m_connectionQ.size()<m_min;});
        if(m_stop)
        break;
        addConnection();
        m_cv.notify_all();
    }
}

void ConnPool::recycleConnection()
{
    while(1)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cv.wait(lock,[this](){return m_stop||m_connectionQ.size()>m_max||m_connectionQ.size()>m_min&&m_connectionQ.front()->getAlivetime()>m_MaxIdleTime;});
        if(m_stop)
        break;
        delete m_connectionQ.front();
        m_connectionQ.pop();
    }
}

std::shared_ptr<MysqlConn> ConnPool::getConnection()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    while(m_connectionQ.empty())
    {
        if(std::cv_status::timeout == m_cv.wait_for(lock,std::chrono::milliseconds(m_timeout)))
        {
            return nullptr;
        }
        else
        continue;
    }
    std::shared_ptr<MysqlConn> pu(m_connectionQ.front(),[this](MysqlConn* mc){
        std::lock_guard<std::mutex> lock(m_mtx);
        m_connectionQ.emplace(mc);
        m_connectionQ.front()->refreshAlivetime();
    });
    m_connectionQ.pop();
    m_cv.notify_all();
    return pu;
}
