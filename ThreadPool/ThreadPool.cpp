#include "ThreadPool.h"

ThreadPool::ThreadPool()
{
    m_thread_num = 0;
    m_act = 1;
}

void ThreadPool::init(int thread_num)
{
    m_thread_num = thread_num;
    m_workers = std::vector<std::thread>(m_thread_num);
    for(int i=0; i<m_thread_num; i++)
    {
        m_workers[i] = std::thread(&ThreadPool::worker,this);
    }
}

ThreadPool::~ThreadPool()
{
    m_act = false;
    m_cv.notify_all();
    for(int i=0; i<m_thread_num; i++)
    {
        if(m_workers[i].joinable())
            m_workers[i].join();
    }
}

ThreadPool& ThreadPool::get_instance()
{
    static ThreadPool tp;
    return tp;
}

void ThreadPool::worker()
{
    while (1)
    {
        std::function<void()> task = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            m_cv.wait(lock,[this](){
                return m_tasks.size()||!m_act;
            });
            if(m_tasks.size())
            {
                task = m_tasks.front();
                m_tasks.pop();
            }
            else
                return;
        }
        task();
    }
}

void ThreadPool::addTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_tasks.emplace(task);
    }
    m_cv.notify_one();
}