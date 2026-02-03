#include "ChunkMeshGenerator.h"
#include "videoWorld/chunk/rendering/ChunkGraphicalData.h"
#include <GlobalVectorPool.h>
#include <iostream>
#include <videoWorld/block/BlockMeshGenerator.h>
#include <utility>

ChunkGraphicalData ChunkMeshGenerator::GetChunkGraphicalData(
    const Chunk& chunk,
    const Chunk* left, const Chunk* right,
    const Chunk* front, const Chunk* back)
{
    VertexDataPair solidBlocksGraphicalData;
    VertexDataPair transparentBlocksGraphicalData;

    solidBlocksGraphicalData.first = std::move(*vertexVectorPool.Acquire());
    solidBlocksGraphicalData.second = std::move(*indexVectorPool.Acquire());

    transparentBlocksGraphicalData.first = std::move(*vertexVectorPool.Acquire());
    transparentBlocksGraphicalData.second = std::move(*indexVectorPool.Acquire());

    unsigned solidBlocksBaseIndex = 0;
    unsigned transparentBlocksBaseIndex = 0;

    for (int x = 0; x < CHUNK_LENGTH; ++x)
    {
        for (int z = 0; z < CHUNK_WIDTH; ++z)
        {
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                const auto block = chunk.GetBlock(x, z, y);

                if (block->blockInfo->block_name == "air")
                    continue;

                const Block* neighbors[6] = { nullptr };

                if (x > 0)
                    neighbors[0] = chunk.GetBlock(x - 1, z, y);
                else
                    if (left)
                        neighbors[0] = left->GetBlock(CHUNK_LENGTH - 1, z, y);

                if (x < CHUNK_LENGTH - 1)
                    neighbors[1] = chunk.GetBlock(x + 1, z, y);
                else
                    if (right)
                        neighbors[1] = right->GetBlock(0, z, y);

                if (z > 0)
                    neighbors[2] = chunk.GetBlock(x, z - 1, y);
                else
                    if (back)
                        neighbors[2] = back->GetBlock(x, CHUNK_WIDTH - 1, y);

                if (z < CHUNK_WIDTH - 1)
                    neighbors[3] = chunk.GetBlock(x, z + 1, y);
                else
                    if (front)
                        neighbors[3] = front->GetBlock(x, 0, y);

                if (y > 0)
                    neighbors[4] = chunk.GetBlock(x, z, y - 1);

                if (y < CHUNK_HEIGHT - 1)
                    neighbors[5] = chunk.GetBlock(x, z, y + 1);

                bool hasVisibleFace = false;
                for (int i = 0; i < 6; ++i) 
                {
                    if (neighbors[i] && neighbors[i]->blockInfo && neighbors[i]->blockInfo->is_transparent)
                    {
                        hasVisibleFace = true;
                        break;
                    }
                }

                if (!hasVisibleFace)
                    continue;

                auto blockMesh = BlockMeshGenerator::GetBlockMeshData(
                    *block,
                    neighbors[2],  // back (Z-1)
                    neighbors[3],  // front (Z+1) 
                    neighbors[0],  // left (X-1)
                    neighbors[1],  // right (X+1)
                    neighbors[4],  // bottom (Y-1)
                    neighbors[5]   // top (Y+1)
                );

                // ссылки, так и нужно тут
          
                VertexDataPair& graphicalData = block->blockInfo->is_transparent ? transparentBlocksGraphicalData: solidBlocksGraphicalData;
                unsigned& baseIndex = block->blockInfo->is_transparent ? transparentBlocksBaseIndex : solidBlocksBaseIndex;

                auto& verts = blockMesh.first;
                auto& inds = blockMesh.second;

                if (verts.empty()) continue;

                const float posX = x + 16 * chunk.x;
                const float posY = static_cast<float>(y);
                const float posZ = z + 16 * chunk.z;

                for (auto& vertex : verts) 
                {
                    vertex.Position.x += posX;
                    vertex.Position.y += posY;
                    vertex.Position.z += posZ;
                }

                graphicalData.first.insert(graphicalData.first.end(), verts.begin(), verts.end());

                for (unsigned int index : inds) 
                {
                    graphicalData.second.push_back(index + baseIndex);
                }

                baseIndex += static_cast<unsigned int>(verts.size());
            }
        }
    }

    ChunkGraphicalData graphicalData = {};
    graphicalData[GeometryType::Solid] = { std::move(solidBlocksGraphicalData), 0 };
    graphicalData[GeometryType::Transparent] = { std::move(transparentBlocksGraphicalData), 0 };
    return graphicalData;
}