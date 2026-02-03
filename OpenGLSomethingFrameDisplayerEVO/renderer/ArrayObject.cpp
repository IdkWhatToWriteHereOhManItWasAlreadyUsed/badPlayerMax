#include "ArrayObject.h"
#include <iostream>


ArrayObject::ArrayObject()
{ 
}

ArrayObject::~ArrayObject() 
{
}

ArrayObject::ArrayObject(ArrayObject&& other) noexcept
{
    glDeleteVertexArrays(1, &m_arrayID);
    m_arrayID = other.m_arrayID;
}

ArrayObject& ArrayObject::operator=(ArrayObject&& other) noexcept
{
    if (this != &other) {
        glDeleteVertexArrays(1, &m_arrayID);
        m_arrayID = other.m_arrayID;
    }
    return *this;
}



void ArrayObject::Create()
{
    glGenVertexArrays(1, &m_arrayID);
    //std::cout << "a " << m_arrayID << '\n';
}

void ArrayObject::Activate()
{
    m_active = true;
    glBindVertexArray(m_arrayID);
}

void ArrayObject::Deactivate()
{
    m_active = false;
    glBindVertexArray(0);
}

bool ArrayObject::IsActive() const 
{
    return m_active;
}

void ArrayObject::AttribPointer(GLuint index, GLint elementsPerVertex, AttribType type, GLsizei stride, GLsizei offset)
{
    m_attribsList.push_back(index);
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, elementsPerVertex, static_cast<GLenum>(type), GL_FALSE, stride, reinterpret_cast<void*>(offset));
}

void ArrayObject::DrawElements(GLint start, GLsizei count)
{
    Activate();
    glDrawElements
    (
        GL_TRIANGLES,
        count,
        GL_UNSIGNED_INT,
        reinterpret_cast<void*>(start)
    );
    Deactivate();
}

void ArrayObject::DisableAttribAll() 
{
    for (GLuint attrib : m_attribsList) 
        glDisableVertexAttribArray(attrib);
}

void ArrayObject::Delete() 
{
    if (m_arrayID == 0) return;
    Deactivate();
    //std::cout << "d " << m_arrayID << '\n';
    glDeleteVertexArrays(1, &m_arrayID);
    m_arrayID = 0;
}