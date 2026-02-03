#include "BlocksGraphicalDataBuffer.h"
#include <videoWorld/chunk/rendering/ChunkGraphicalData.h>
#include <iostream>
#include <utility>
#include <vector>
#include <mutex>

BlocksGraphicalDataBuffer::BlocksGraphicalDataBuffer()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_graphicalData.reserve(6222);
    m_pending_writes.reserve(6222);
    m_pending_ids.reserve(6222);

    m_graphicalData.resize(6222);

    // тут так и должна быть 1
    // без нее самый первый чанк почему-то не удаляется
    // я не помню в чём заключается прикол
    // когда-то это обязательно будет починено
    for (int i = 6221; i >= 1; --i)
        m_indicesToAdd.push(i);
    m_maxIndexToAdd = 6222;
}

GLuint BlocksGraphicalDataBuffer::Write(VertexDataPair data, AABB aabb)
{
    GLuint id;
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_indicesToAdd.empty())
    {
        id = m_indicesToAdd.top();
        m_indicesToAdd.pop();
    }
    else
        id = m_maxIndexToAdd++;

   // if (!m_graphicalData[id].isDeleted)
   // m_graphicalData[id].mesh.Delete();

    m_pending_writes.push_back({ std::move(data), aabb, id });
    //std::cout << id << '\n';
    return id;
}

void BlocksGraphicalDataBuffer::DeleteAt(GLuint id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!id) return;
  
    if (id < m_graphicalData.size() && !m_graphicalData[id].isDeleted)
        m_indicesToDelete.push(id);
}

void BlocksGraphicalDataBuffer::Clear()
{
    // не будет
}

std::vector<GraphicalBufferItem>& BlocksGraphicalDataBuffer::LoadGraphicalData()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    while (!m_indicesToDelete.empty())
    {
        GLuint id = m_indicesToDelete.top();
        m_indicesToDelete.pop();
        if (id < m_graphicalData.size() && !m_graphicalData[id].isDeleted)
        {
            m_graphicalData[id].mesh.Delete();
            m_graphicalData[id].isDeleted = true;
            m_indicesToAdd.push(id);
        }
    }

    for (auto& pending : m_pending_writes)
    {
        GLuint id = pending.id;
        if (id >= m_graphicalData.size())
            m_graphicalData.resize(id + 1);

        m_graphicalData[id].mesh.Create();
        m_graphicalData[id].mesh.SetData(pending.vertexData.first, pending.vertexData.second);
        m_graphicalData[id].mesh.SetupMesh();
        m_graphicalData[id].aabb = pending.aabb;
        m_graphicalData[id].isDeleted = false;
    }

    m_pending_writes.clear();
    return m_graphicalData;
}