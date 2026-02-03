#pragma once

#include <vector>
#include <queue>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "Task.h"

class ThreadPool
{
private:
    struct TaskWrapper
    {
        std::shared_ptr<IInvokable> task;
        std::atomic<bool> completed{false};
        std::condition_variable cv;
    };

    std::vector<std::thread> m_workerThreads;
    std::queue<size_t> m_taskQueue;
    std::unordered_map<size_t, std::shared_ptr<TaskWrapper>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    std::condition_variable m_waitAllCV;
    std::atomic<size_t> m_nextTaskID;
    std::atomic<bool> m_needStop;
    std::atomic<size_t> m_activeTasks{0};

    void ThreadProc();
    
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    size_t AddTask(std::shared_ptr<IInvokable> task);
    void Wait(size_t task_id);
    void WaitAll();
    bool IsTaskComplete(size_t task_id);
    void Shutdown();
    int GetThreadCount() { return m_workerThreads.size(); }
    void SetThreadCount(int count);

private:
    void CleanupCompletedTasks();
};