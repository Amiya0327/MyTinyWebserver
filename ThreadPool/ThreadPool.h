#pragma once
#include<iostream>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>
#include <unordered_map>

class ThreadPool
{
public:

    static ThreadPool& get_instance();

    ThreadPool(const ThreadPool& tp) = delete;

    ThreadPool& operator=(const ThreadPool& tp) = delete;

    //添加任务
    void addTask(std::function<void()> task);

private:

    ~ThreadPool();

    ThreadPool(int min = 8, int max = std::thread::hardware_concurrency());

    void manager();

    void worker();

    std::unordered_map<std::thread::id, std::thread> m_workers; //工作线程
    std::thread *m_manager; //管理线程
    std::vector<std::thread::id> m_ids; //待销毁线程id
    std::atomic<int> m_minThreadNum; //最小线程数量
    std::atomic<int> m_maxThreadNum; //最大线程数量
    std::atomic<int> m_idleThreadNum; //空闲线程数量
    std::atomic<int> m_curTheadNum; //当前线程数量
    std::atomic<int> m_exitThreadNum; //待退出线程数量
    std::atomic<bool> m_act; //线程开关
    std::queue<std::function<void()>> m_tasks; //任务队列
    std::mutex m_mtx;
    std::mutex m_id;
    std::condition_variable m_cv;
};
