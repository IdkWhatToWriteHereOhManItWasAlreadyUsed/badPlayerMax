#include "ChunkInstance.h"
#include "rendering/ChunkMeshGenerator.h"
#include "rendering/ChunkGraphicalData.h"
#include <memory>
#include <videoWorld/block/Block.h>
#include "model/Chunk.h"

ChunkInstance::ChunkInstance(int x, int z)
{
    m_chunk = std::make_unique<Chunk>();
    m_chunk->x = x;
    m_chunk->z = z;
    m_graphicalData.setAABB(x, z);
}

ChunkInstance::~ChunkInstance()
{
}

void ChunkInstance::SetBlock(int x, int y, int z, const Block& block)
{
    m_chunk->SetBlock(x, z, y, block);
    InvalidateMeshData();
}

Block* ChunkInstance::GetBlock(int x, int y, int z)
{
    return m_chunk->GetBlock(x, z, y);
}

ChunkGraphicalData& ChunkInstance::GetGraphicalData()
{
    return m_graphicalData;
}

void ChunkInstance::GenerateGraphicalDataIfNeeded(ChunkInstance* left, ChunkInstance* right, ChunkInstance* front, ChunkInstance* back)
{
    if (!m_isMeshDataValid)
    {
        m_graphicalData = ChunkMeshGenerator::GetChunkGraphicalData
        (
            *m_chunk, 
            (left != nullptr) ? left->m_chunk.get(): nullptr,
            (right != nullptr) ? right->m_chunk.get() : nullptr,
            (front != nullptr) ? front->m_chunk.get() : nullptr,
            (back != nullptr) ? back->m_chunk.get() : nullptr 
        );
        m_graphicalData.setAABB(m_chunk->x, m_chunk->z);

        m_isMeshDataValid = true;
    }
}