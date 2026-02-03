#pragma once

#include <stdexcept>
#include <type_traits>
#include <iostream>
#include <tuple>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <chrono>

class IInvokable
{
public:
    virtual ~IInvokable() = default;
    virtual unsigned long Run() = 0;
    virtual bool isComplete() const = 0;
    virtual bool hasStarted() const = 0;
};

template <typename TResult, typename... Args>
class Task : public IInvokable
{
private:
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;
    bool m_hasStarted{ false };
    bool m_isComplete{ false };
    std::function<TResult(Args...)> m_task;
    std::tuple<Args...> m_args;
    TResult m_result;
    std::exception_ptr m_exception;

public:
    Task(std::function<TResult(Args...)> task, Args... args) :
        m_task(std::move(task)),
        m_args(std::make_tuple(args...))
    {
    }

    // Move constructor
    Task(Task&& other) noexcept
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_task = std::move(other.m_task);
        m_args = std::move(other.m_args);
        m_result = std::move(other.m_result);
        m_hasStarted = other.m_hasStarted;
        m_isComplete = other.m_isComplete;
        m_exception = std::move(other.m_exception);
    }

    // Move assignment operator
    Task& operator=(Task&& other) noexcept
    {
        if (this != &other) {
            std::scoped_lock lock(m_mutex, other.m_mutex);
            m_task = std::move(other.m_task);
            m_args = std::move(other.m_args);
            m_result = std::move(other.m_result);
            m_hasStarted = other.m_hasStarted;
            m_isComplete = other.m_isComplete;
            m_exception = std::move(other.m_exception);
        }
        return *this;
    }

    // Удаляем конструктор копирования и оператор присваивания
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    ~Task() = default;

private:
    template <typename F, typename Tuple, std::size_t... I>
    auto apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
        return f(std::get<I>(std::forward<Tuple>(t))...);
    }

    template <typename F, typename Tuple>
    auto my_apply(F&& f, Tuple&& t) {
        using Indices = std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>;
        return apply_impl(std::forward<F>(f), std::forward<Tuple>(t), Indices{});
    }

public:
    unsigned long Run() override
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_hasStarted = true;
        }

        try
        {
            m_result = my_apply(m_task, m_args);
            m_exception = nullptr;
        }
        catch (...)
        {
            m_exception = std::current_exception();
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_isComplete = true;
            m_cv.notify_all();
        }

        return (m_exception == nullptr) ? 0 : 1;
    }

    TResult Result()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]() { return m_isComplete; });
        
        if (m_exception) {
            std::rethrow_exception(m_exception);
        }
        
        return std::move(m_result);
    }

    bool isComplete() const override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_isComplete;
    }

    bool hasStarted() const override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_hasStarted;
    }
};

template <typename... Args>
class Task<void, Args...> : public IInvokable
{
private:
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;
    bool m_hasStarted{ false };
    bool m_isComplete{ false };
    std::function<void(Args...)> m_task;
    std::tuple<Args...> m_args;
    std::exception_ptr m_exception;

public:
    Task(std::function<void(Args...)> task, Args... args) :
        m_task(std::move(task)),
        m_args(std::make_tuple(args...))
    {
    }

    Task(Task&& other) noexcept
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_task = std::move(other.m_task);
        m_args = std::move(other.m_args);
        m_hasStarted = other.m_hasStarted;
        m_isComplete = other.m_isComplete;
        m_exception = std::move(other.m_exception);
    }

    Task& operator=(Task&& other) noexcept
    {
        if (this != &other) {
            std::scoped_lock lock(m_mutex, other.m_mutex);
            m_task = std::move(other.m_task);
            m_args = std::move(other.m_args);
            m_hasStarted = other.m_hasStarted;
            m_isComplete = other.m_isComplete;
            m_exception = std::move(other.m_exception);
        }
        return *this;
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    ~Task() = default;

private:
    template <typename F, typename Tuple, std::size_t... I>
    void apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
    {
        f(std::get<I>(std::forward<Tuple>(t))...);
    }

    template <typename F, typename Tuple>
    void my_apply(F&& f, Tuple&& t)
    {
        using Indices = std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>;
        apply_impl(std::forward<F>(f), std::forward<Tuple>(t), Indices{});
    }

public:
    unsigned long Run() override
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_hasStarted = true;
        }

        try
        {
            my_apply(m_task, m_args);
            m_exception = nullptr;
        }
        catch (...)
        {
            m_exception = std::current_exception();
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_isComplete = true;
            m_cv.notify_all();
        }

        return (m_exception == nullptr) ? 0 : 1;
    }

    void Result()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]() { return m_isComplete; });
        
        if (m_exception) {
            std::rethrow_exception(m_exception);
        }
    }

    bool isComplete() const override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_isComplete;
    }

    bool hasStarted() const override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_hasStarted;
    }
};