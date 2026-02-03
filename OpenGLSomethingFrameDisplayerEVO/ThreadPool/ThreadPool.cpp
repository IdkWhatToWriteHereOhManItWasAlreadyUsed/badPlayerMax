#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t num_threads) : m_nextTaskID{0}, m_needStop{false}
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        m_workerThreads.emplace_back(&ThreadPool::ThreadProc, this);
    }
}

ThreadPool::~ThreadPool()
{
    Shutdown();
    for (auto& thread : m_workerThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

size_t ThreadPool::AddTask(std::shared_ptr<IInvokable> task)
{
    std::unique_lock<std::mutex> lock(m_queueMutex);
    
    if (m_needStop) {
        return static_cast<size_t>(-1); // Возвращаем невалидный ID если пул остановлен
    }
    
    size_t id = m_nextTaskID.fetch_add(1);
    ++m_activeTasks;

    auto wrapper = std::make_shared<TaskWrapper>();
    wrapper->task = task;

    m_tasks[id] = wrapper;
    m_taskQueue.push(id);
    
    lock.unlock();
    m_queueCV.notify_one();
    return id;
}

void ThreadPool::SetThreadCount(int count)
{
    if (count < 1) return;
    
    Shutdown();
    
    // Ждем завершения всех текущих потоков
    for (auto& thread : m_workerThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    
    // Очищаем задачи, которые были в очереди, но не выполнялись
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_taskQueue.empty())
        {
            size_t task_id = m_taskQueue.front();
            m_taskQueue.pop();
            
            auto it = m_tasks.find(task_id);
            if (it != m_tasks.end())
            {
                it->second->completed = true;
                --m_activeTasks;
            }
        }
        
        // Очищаем map от старых задач
        for (auto it = m_tasks.begin(); it != m_tasks.end();)
        {
            if (it->second->completed)
            {
                it = m_tasks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    
    // Создаем новые потоки
    m_needStop = false;
    m_workerThreads.clear();
    for (size_t i = 0; i < count; ++i)
    {
        m_workerThreads.emplace_back(&ThreadPool::ThreadProc, this);
    }
}

void ThreadPool::Wait(size_t task_id)
{
    std::shared_ptr<TaskWrapper> wrapper;
    
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        auto it = m_tasks.find(task_id);
        if (it == m_tasks.end())
        {
            return; // Задача уже завершена и удалена
        }
        wrapper = it->second;
    }
    
    std::unique_lock<std::mutex> lock(m_queueMutex);
    wrapper->cv.wait(lock, [wrapper]() {
        return wrapper->completed.load();
    });
}

void ThreadPool::WaitAll()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_waitAllCV.wait(lock, [this]() {
        return m_activeTasks.load() == 0 && m_taskQueue.empty();
    });
}

bool ThreadPool::IsTaskComplete(size_t task_id)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    auto it = m_tasks.find(task_id);
    if (it == m_tasks.end())
    {
        return true; // Задача уже удалена (значит завершена)
    }
    return it->second->completed.load();
}

void ThreadPool::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_needStop = true;
        m_queueCV.notify_all();
    }
}

void ThreadPool::CleanupCompletedTasks()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    for (auto it = m_tasks.begin(); it != m_tasks.end();)
    {
        if (it->second->completed)
        {
            it = m_tasks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ThreadPool::ThreadProc()
{
    while (true)
    {
        size_t task_id = 0;
        std::shared_ptr<TaskWrapper> wrapper;
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this]() {
                return m_needStop || !m_taskQueue.empty();
            });

            if (m_needStop && m_taskQueue.empty())
            {
                break;
            }

            task_id = m_taskQueue.front();
            m_taskQueue.pop();
            
            auto it = m_tasks.find(task_id);
            if (it != m_tasks.end())
            {
                wrapper = it->second;
            }
        }

        if (wrapper && !wrapper->completed)
        {
            try
            {
                wrapper->task->Run();
            }
            catch (...)
            {
                // Игнорируем исключения из задач
            }
            
            wrapper->completed = true;
            --m_activeTasks;
            
            // Уведомляем ожидающих
            wrapper->cv.notify_all();
            
            // Если больше нет активных задач, уведомляем WaitAll
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (m_activeTasks == 0 && m_taskQueue.empty())
                {
                    m_waitAllCV.notify_all();
                }
            }
        }
        
        // Периодически чистим завершенные задачи
        static thread_local int cleanupCounter = 0;
        if (++cleanupCounter % 100 == 0)
        {
            CleanupCompletedTasks();
        }
    }
}