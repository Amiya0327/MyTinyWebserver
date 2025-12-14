#include "ThreadPool.h"

ThreadPool::ThreadPool(int min, int max) : m_maxThreadNum(max), m_minThreadNum(min), m_act(true), m_idleThreadNum(min), m_curTheadNum(min)
{
    m_manager = new std::thread(&ThreadPool::manager, this);
    m_exitThreadNum.store(0);
    for (int i = 0; i < min; i++)
    {
        auto t = std::thread(&ThreadPool::worker, this);
        m_workers.insert(std::pair<std::thread::id, std::thread>(t.get_id(), move(t)));
    }
}

ThreadPool::~ThreadPool()
{
    if(m_act==0)
    return;
    m_act = false;
    m_cv.notify_all();
    for (auto &worker : m_workers)
    {
        if (worker.second.joinable())
        {
            //cout << "线程：" << worker.second.get_id() << "将要退出" << endl;
            worker.second.join();
        }
    }
    if (m_manager->joinable())
    {
        m_manager->join();
    }
    delete m_manager;
}

ThreadPool& ThreadPool::get_instance()
{
    static ThreadPool tp;
    return tp;
}

void ThreadPool::manager()
{
    while (m_act)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int idle = m_idleThreadNum.load();
        int cur = m_curTheadNum.load();
        if (idle > cur / 2 && cur > m_minThreadNum + 1)
        {
            m_exitThreadNum.store(2);
            m_cv.notify_all();
            {
                std::lock_guard<std::mutex> lock(m_id);
                for (auto &id : m_ids)
                {
                    auto it = m_workers.find(id);
                    if (it != m_workers.end())
                    {
                        //cout << "线程：" << it->second.get_id() << "销毁" << endl;
                        it->second.join();
                        m_workers.erase(id);
                        m_curTheadNum--;
                        m_idleThreadNum--;
                    }
                }

                m_ids.clear();
            }
        }
        if (idle == 0 && cur < m_maxThreadNum)
        {
            auto t = std::thread(&ThreadPool::worker, this);
            m_workers.insert(std::pair<std::thread::id, std::thread>(t.get_id(), move(t)));
            //cout << "增加了一个线程" << endl;
            m_curTheadNum++;
            m_idleThreadNum++;
        }
    }
}

void ThreadPool::worker()
{
    while (1)
    {
        std::function<void()> task = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            m_cv.wait(lock, [this]()
                      { return !m_tasks.empty() || !m_act || m_exitThreadNum; });
            if (m_exitThreadNum)
            {
                m_exitThreadNum--;
                {
                    std::lock_guard<std::mutex> lock(m_id);
                    m_ids.emplace_back(std::this_thread::get_id());
                }
                return;
            }
            if (!m_tasks.empty())
            {
                task = move(m_tasks.front());
                m_tasks.pop();
            }
            else if (!m_act)
                return;
        }
        if (task)
        {
            //cout << "取出了一个任务" << endl;
            m_idleThreadNum--;
            task();
            m_idleThreadNum++;
        }
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