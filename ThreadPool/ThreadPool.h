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

    void init(int thread_num);

    ThreadPool(const ThreadPool& tp) = delete;

    ThreadPool& operator=(const ThreadPool& tp) = delete;

    //添加任务
    void addTask(std::function<void()> task);

private:

    ~ThreadPool();

    //ThreadPool(int min = 8, int max = std::thread::hardware_concurrency());
    ThreadPool();

    void worker();

    std::vector<std::thread> m_workers;
    int m_thread_num;
    bool m_act; //线程开关
    std::queue<std::function<void()>> m_tasks; //任务队列

    //队列锁
    std::mutex m_mtx;
    std::condition_variable m_cv;
};
