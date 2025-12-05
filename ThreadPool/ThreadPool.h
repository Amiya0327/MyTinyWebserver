#pragma once
#include<iostream>
using namespace std;
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
    void addTask(function<void()> task);

private:

    ~ThreadPool();

    ThreadPool(int min = 8, int max = thread::hardware_concurrency());

    void manager();

    void worker();

    unordered_map<thread::id, thread> m_workers; //工作线程
    thread *m_manager; //管理线程
    vector<thread::id> m_ids; //待销毁线程id
    atomic<int> m_minThreadNum; //最小线程数量
    atomic<int> m_maxThreadNum; //最大线程数量
    atomic<int> m_idleThreadNum; //空闲线程数量
    atomic<int> m_curTheadNum; //当前线程数量
    atomic<int> m_exitThreadNum; //待退出线程数量
    atomic<bool> m_act; //线程开关
    queue<function<void()>> m_tasks; //任务队列
    mutex m_mtx;
    mutex m_id;
    condition_variable m_cv;
};
