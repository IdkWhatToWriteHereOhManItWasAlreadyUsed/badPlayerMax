#pragma once

#include <vector>
#include "ArrayObject.h"
#include "BufferObject.h"
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh
{
public:
    Mesh();
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void Create();
    void SetData(std::vector<Vertex> vertices, std::vector<GLuint> indices);
    void SetupMesh();
    void Draw();
    void Delete();

    std::vector<Vertex>& getVertices() { return m_vertices; }
    std::vector<GLuint>& getIndices() { return m_indices; }

private:
    ArrayObject m_VAO = ArrayObject();
    BufferObject m_VBO = BufferObject(BufferType::ArrayBuffer);
    BufferObject m_EBO = BufferObject(BufferType::ElementBuffer);

    std::vector<Vertex> m_vertices;
    std::vector<GLuint> m_indices;
};