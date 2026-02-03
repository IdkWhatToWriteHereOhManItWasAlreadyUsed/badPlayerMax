#include "Mesh.h"
#include "ArrayObject.h"
#include <vector>
#include <GlobalVectorPool.h>

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_VAO(std::move(other.m_VAO)),
    m_VBO(std::move(other.m_VBO)),
    m_EBO(std::move(other.m_EBO)),
    m_vertices(std::move(other.m_vertices)),
    m_indices(std::move(other.m_indices))
{
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        m_VAO = std::move(other.m_VAO);
        m_VBO = std::move(other.m_VBO);
        m_EBO = std::move(other.m_EBO);
        m_vertices = std::move(other.m_vertices);
        m_indices = std::move(other.m_indices);
    }
    return *this;
}

void Mesh::SetupMesh()
{
    m_VAO.Activate();
    m_VBO.SetData(m_vertices, GL_DYNAMIC_DRAW);
    m_EBO.SetData(m_indices, GL_DYNAMIC_DRAW);

    m_VAO.AttribPointer(0, 3, AttribType::Float, sizeof(Vertex), offsetof(Vertex, Position));
    m_VAO.AttribPointer(1, 3, AttribType::Float, sizeof(Vertex), offsetof(Vertex, Normal));
    m_VAO.AttribPointer(2, 2, AttribType::Float, sizeof(Vertex), offsetof(Vertex, TexCoords));
    m_VAO.Deactivate();
}

void Mesh::Create()
{
    m_VAO.Create();
    m_VBO.Create();
    m_EBO.Create();
}

void Mesh::SetData(std::vector<Vertex> vertices, std::vector<GLuint> indices)
{
    m_vertices = std::move(vertices);
    m_indices = std::move(indices);
}

void Mesh::Draw()
{
    m_VAO.DrawElements(0, m_indices.size());
}

void Mesh::Delete()
{
    m_VAO.Delete();
    m_VBO.Delete();
    m_EBO.Delete();

    vertexVectorPool.Release(&m_vertices);
    indexVectorPool.Release(&m_indices);
    //m_vertices.clear();
    //m_indices.clear();
}