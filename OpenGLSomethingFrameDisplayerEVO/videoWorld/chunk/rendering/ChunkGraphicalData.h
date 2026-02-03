#pragma once

#include <GL/glew.h>
#include <utility>
#include <vector>
#include <renderer/Mesh.h>
#include "videoWorld/chunk/model/Chunk.h"
#include <array>

using VertexDataPair = std::pair<std::vector<Vertex>, std::vector<GLuint>>;
typedef std::pair<glm::vec3, glm::vec3> AABB;

struct MeshData
{
    VertexDataPair Data;
    GLuint IDinBuffer = 0;
};

enum class GeometryType: std::size_t
{
    Solid,
    Transparent, 
    //Liquid, 
    //
    Count 
};

struct ChunkGraphicalData
{
    std::array<MeshData, static_cast<std::size_t>(GeometryType::Count)> allBlockTypesMeshData = {};
    AABB aabb;
    void setAABB(int chunkX, int chunkZ)
    {
        aabb = std::pair<glm::vec3, glm::vec3>
        (
            glm::vec3(chunkX * CHUNK_LENGTH, 0, chunkZ * CHUNK_WIDTH),
            glm::vec3(chunkX * CHUNK_LENGTH + CHUNK_LENGTH, CHUNK_HEIGHT, chunkZ * CHUNK_WIDTH + CHUNK_WIDTH)
        );
    }

    MeshData& operator [] (const GeometryType t) { return allBlockTypesMeshData[static_cast<std::size_t>(t)]; }
    const MeshData& operator [] (const GeometryType t) const { return allBlockTypesMeshData[static_cast<std::size_t>(t)]; }

    const MeshData& operator [] (const std::size_t t) const { return allBlockTypesMeshData[t]; }
};
