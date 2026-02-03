#pragma once

#include <utility>
#include <mutex>
#include <renderer/Mesh.h>
#include <stack>
#include <vector>
#include <videoWorld/chunk/rendering/ChunkGraphicalData.h>
#include <libs/robin_hood.h>
#include <GL/glew.h>

using VertexDataPair = std::pair<std::vector<Vertex>, std::vector<GLuint>>;

struct GraphicalBufferItem
{
    Mesh mesh;
    AABB aabb;
    bool isDeleted = true;
};

struct PendingWriteData 
{
    VertexDataPair vertexData;
    AABB aabb;
    GLuint id;
};

class BlocksGraphicalDataBuffer
{
private:
    using PendingWritesVector = std::vector<PendingWriteData>;

public:
    BlocksGraphicalDataBuffer();

    BlocksGraphicalDataBuffer(const BlocksGraphicalDataBuffer&) = delete;
    BlocksGraphicalDataBuffer& operator=(const BlocksGraphicalDataBuffer&) = delete;
    BlocksGraphicalDataBuffer(BlocksGraphicalDataBuffer&& other) noexcept
        : m_graphicalData(std::move(other.m_graphicalData))
        , m_pending_writes(std::move(other.m_pending_writes))
        , m_pending_ids(std::move(other.m_pending_ids))
        , m_indicesToDelete(std::move(other.m_indicesToDelete))
        , m_indicesToAdd(std::move(other.m_indicesToAdd))
        , m_maxIndexToAdd(other.m_maxIndexToAdd)
    {
    }
    
    GLuint Write(VertexDataPair data, AABB aabb);
    void DeleteAt(GLuint id);
    void Clear();
    std::vector<GraphicalBufferItem>& LoadGraphicalData();

    class Iterator {
    public:
        Iterator(std::vector<GraphicalBufferItem>& items, size_t index)
            : m_items(items), m_index(index)
        {
            skipDeleted();
        }

        GraphicalBufferItem& operator*() { return m_items[m_index]; }
        GraphicalBufferItem* operator->() { return &m_items[m_index]; }

        Iterator& operator++()
        {
            ++m_index;
            skipDeleted();
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const
        {
            return m_index == other.m_index;
        }

        bool operator!=(const Iterator& other) const
        {
            return m_index != other.m_index;
        }

    private:
        std::vector<GraphicalBufferItem>& m_items;
        size_t m_index;

        void skipDeleted()
        {
            while (m_index < m_items.size() && m_items[m_index].isDeleted)
            {
                ++m_index;
            }
        }
    };

    Iterator begin()
    {
        auto& items = LoadGraphicalData();
        return Iterator(items, 0);
    }

    Iterator end()
    {
        auto& items = LoadGraphicalData();
        return Iterator(items, items.size());
    }

    class PendingWritesIterator {
    public:
        PendingWritesIterator(PendingWritesVector::iterator it) : m_it(it) {}

        auto& operator*() { return *m_it; }
        auto* operator->() { return &(*m_it); }

        PendingWritesIterator& operator++() { ++m_it; return *this; }
        PendingWritesIterator operator++(int) { auto tmp = *this; ++m_it; return tmp; }

        bool operator==(const PendingWritesIterator& other) const { return m_it == other.m_it; }
        bool operator!=(const PendingWritesIterator& other) const { return m_it != other.m_it; }

    private:
        PendingWritesVector::iterator m_it;
    };

    PendingWritesIterator pending_begin()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return PendingWritesIterator(m_pending_writes.begin());
    }

    PendingWritesIterator pending_end()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return PendingWritesIterator(m_pending_writes.end());
    }

    size_t pending_size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pending_writes.size();
    }

private:
    std::vector<GraphicalBufferItem> m_graphicalData;
    PendingWritesVector m_pending_writes;
    robin_hood::unordered_flat_set<GLuint> m_pending_ids;

    std::stack<GLuint> m_indicesToDelete;
    std::stack<GLuint> m_indicesToAdd;
    GLuint m_maxIndexToAdd = 0;
    mutable std::mutex m_mutex;
};