#include "BufferObject.h"

BufferObject::BufferObject(BufferType type)
    : m_type(static_cast<GLenum>(type))
{

}

BufferObject::~BufferObject() 
{
}

BufferObject::BufferObject(BufferObject&& other) noexcept
{
   // glGenBuffers(1, &m_bufferID);
       // std::cout << "add " << m_arrayID << '\n';
    glDeleteVertexArrays(1, &m_bufferID);
    m_bufferID = other.m_bufferID;
    m_type = other.m_type;
}

BufferObject& BufferObject::operator=(BufferObject&& other) noexcept
{
    if (this != &other) {
       // glGenBuffers(1, &m_bufferID);
      //  std::cout << "add " << m_arrayID << '\n';
        glDeleteVertexArrays(1, &m_bufferID);
        m_bufferID = other.m_bufferID;
        m_type = other.m_type;
    }
    return *this;
}

void BufferObject::Create()
{
    glGenBuffers(1, &m_bufferID);
}

void BufferObject::Activate()
{
    glBindBuffer(m_type, m_bufferID);
}

void BufferObject::Deactivate()
{
    glBindBuffer(m_type, 0);
}

void BufferObject::Delete()
{
    if (m_bufferID)
    {
        Deactivate();
        glDeleteBuffers(1, &m_bufferID);
        m_bufferID = 0;
    }
}