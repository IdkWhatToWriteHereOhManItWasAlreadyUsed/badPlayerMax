#pragma once

#include <vector>
#include <GL/glew.h> 

enum class AttribType
{
    Float = GL_FLOAT
};

class BufferObject;

class ArrayObject 
{
public:
    ArrayObject();
    ~ArrayObject();
    ArrayObject(ArrayObject&& other) noexcept;
    ArrayObject& operator=(ArrayObject&& other) noexcept;

    void Create();
    void Activate();
    void Deactivate();
    bool IsActive() const;

    void AttribPointer(GLuint index, GLint elementsPerVertex, AttribType type, GLsizei stride, GLsizei offset);

    void DrawElements(GLint start, GLsizei count);

    void DisableAttribAll();
    void Delete();

private:
    GLuint m_arrayID = 0;
    bool m_active = false;
    std::vector<GLuint> m_attribsList;
};