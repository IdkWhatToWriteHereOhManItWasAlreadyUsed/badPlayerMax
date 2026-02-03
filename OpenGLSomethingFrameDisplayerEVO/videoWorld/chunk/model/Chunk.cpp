#include "Chunk.h"

Chunk::Chunk()
{
}

Chunk::~Chunk()
{
}

void Chunk::SetBlock(int x, int z, int y, const Block block)
{
    m_blocks[x][z][y] = block;
}

Block* Chunk::GetBlock(int x, int z, int y) const
{
    return x < CHUNK_WIDTH && x >= 0 
        && y < CHUNK_HEIGHT && y >= 0 
        && z < CHUNK_LENGTH && z >= 0
        ? const_cast<Block*>(&m_blocks[x][z][y])
        : nullptr;
}