#pragma once


#include <GL/glew.h>

#include <vector>

enum class BufferType 
{
    ArrayBuffer = GL_ARRAY_BUFFER,
    ElementBuffer = GL_ELEMENT_ARRAY_BUFFER
};

enum class DrawMode : int 
{
    STATIC = GL_STATIC_DRAW,
    DYNAMIC = GL_DYNAMIC_DRAW,
    STREAM = GL_STREAM_DRAW
};

class BufferObject
{
public:
    BufferObject(BufferType type);
    ~BufferObject();
    BufferObject(BufferObject&& other) noexcept;
    BufferObject& operator=(BufferObject&& other) noexcept;

    template<typename T>
    void SetData(const std::vector<T>& data, GLenum hint)
    {
        if (data.empty())
            return;

        Activate();
        glBufferData(m_type, data.size() * sizeof(T), data.data(), hint);
    }

    void Create();
    void Activate();
    void Deactivate();

    void Delete();

private:
    GLuint m_bufferID = 0;
    GLenum m_type;
};