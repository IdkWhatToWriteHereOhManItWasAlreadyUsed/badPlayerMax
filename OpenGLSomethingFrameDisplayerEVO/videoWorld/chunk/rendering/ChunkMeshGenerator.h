#pragma once

#include <renderer/Mesh.h>
#include <videoWorld/chunk/model/Chunk.h>
#include <videoWorld/chunk/rendering/ChunkGraphicalData.h>

class ChunkMeshGenerator
{
public:
    static ChunkGraphicalData GetChunkGraphicalData(const Chunk& chunk, const Chunk* left, const Chunk* right, const Chunk* front, const Chunk* back);
};