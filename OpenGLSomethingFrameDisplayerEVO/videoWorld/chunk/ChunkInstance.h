#pragma once

#include <memory>
#include <vector>
#include <utility> 

#include "model/Chunk.h"
#include "rendering/ChunkGraphicalData.h"
#include "videoWorld/block/Block.h"
#include <renderer/Mesh.h>

using VertexDataPair = std::pair<std::vector<Vertex>, std::vector<GLuint>>;

class ChunkInstance
{
private:
    std::unique_ptr<Chunk> m_chunk;

    ChunkGraphicalData m_graphicalData;
    bool m_isMeshDataValid = false;
public:
    ChunkInstance(int x, int z);
    ~ChunkInstance();
    void SetBlock(int x, int y, int z, const Block& block);
    Block* GetBlock(int x, int y, int z);
    int GetChunkX() { return m_chunk->x; }
    int GetChunkZ() { return m_chunk->z; }
    GLuint GetMeshID(GeometryType geometryType) { return m_graphicalData[geometryType].IDinBuffer; }
    GLuint GetMeshID(int geometryType) const { return m_graphicalData[geometryType].IDinBuffer; }
    void SetMeshID(GeometryType geometryType, GLuint id) { m_graphicalData[geometryType].IDinBuffer = id; }
    void SetMeshID(int index, GLuint id) { m_graphicalData.allBlockTypesMeshData[index].IDinBuffer = id; }

    ChunkGraphicalData& GetGraphicalData();
    AABB getAABB() const { return m_graphicalData.aabb; }

    bool IsMeshValid() const { return m_isMeshDataValid; }
    void InvalidateMeshData() { m_isMeshDataValid = false; }
    void GenerateGraphicalDataIfNeeded(ChunkInstance* left, ChunkInstance* right, ChunkInstance* front, ChunkInstance* back);

};