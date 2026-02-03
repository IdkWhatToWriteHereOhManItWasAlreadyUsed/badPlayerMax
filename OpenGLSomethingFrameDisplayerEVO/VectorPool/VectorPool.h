#pragma once

#include <vector>
#include <mutex>
#include <stack>

template <typename T>
class VectorPool
{
public:
    explicit VectorPool(size_t vector_capacity)
        : m_capacity(vector_capacity)
    {
    }

    std::vector<T>* Acquire()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_free.empty())
        {
            auto* vec = new std::vector<T>();
            vec->reserve(m_capacity);
            m_all_vectors.push_back(vec);
            return vec;
        }
        
        auto* vec = m_free.top();
        m_free.pop();
        vec->clear();
        return vec;
    }

    void Release(std::vector<T>* vec)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        vec->clear();
        m_free.push(vec);
    }

    size_t TotalCreated() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_all_vectors.size();
    }

    size_t Available() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_free.size();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_free.empty())
        {
            m_free.pop();
        }
        for (auto* vec : m_all_vectors)
        {
            delete vec;
        }
        m_all_vectors.clear();
    }

    ~VectorPool()
    {
        Clear();
    }

private:
    std::vector<std::vector<T>*> m_all_vectors;
    std::stack<std::vector<T>*> m_free;
    mutable std::mutex m_mutex;
    size_t m_capacity;
};