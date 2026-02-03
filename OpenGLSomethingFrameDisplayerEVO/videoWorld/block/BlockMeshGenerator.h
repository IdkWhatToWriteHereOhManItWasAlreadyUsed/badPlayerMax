/*#pragma once
#include <vector>
#include <array>
#include <renderer/Mesh.h>
#include "Block.h"

class BlockMeshGenerator
{
public:
    static std::pair<std::vector<Vertex>, std::vector<GLuint>> GetBlockMeshData(
        const Block& block,
        const Block* back,
        const Block* front,
        const Block* left,
        const Block* right,
        const Block* bottom,
        const Block* top
    );
private:
};

*/
#pragma once
#include <vector>
#include <array>
#include <renderer/Mesh.h>
#include "Block.h"
#include <videoWorld/chunk/rendering/ChunkGraphicalData.h>
#include <unordered_map>

class BlockMeshGenerator
{
public:
    static std::unordered_map<std::string, VertexDataPair> m_cachedGeometryMeshes;

public:
    
    static void InitialiseCachesMeshes();
    static std::pair<std::vector<Vertex>, std::vector<GLuint>> GetBlockMeshData(
        const Block& block,
        const Block* back,
        const Block* front,
        const Block* left,
        const Block* right,
        const Block* bottom,
        const Block* top
    );
};