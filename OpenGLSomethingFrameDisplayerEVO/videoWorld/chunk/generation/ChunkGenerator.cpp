#include "ChunkGenerator.h"
#include <algorithm>
#include <videoWorld/chunk/model/Chunk.h>
#include <videoWorld/chunk/ChunkInstance.h>
#include <functional>
#include <cmath>

std::unordered_map<std::string, const BlockInfo*> ChunkGenerator::m_blockCache;
std::mt19937 ChunkGenerator::m_randomEngine(std::random_device{}());


void ChunkGenerator::InitializeBlocks()
{
    const BlockInfo* airInfo = BlocksManager::GetBlockInfoByName("air");
    const BlockInfo* grassInfo = BlocksManager::GetBlockInfoByName("grass_block");
    const BlockInfo* dirtInfo = BlocksManager::GetBlockInfoByName("dirt");
    const BlockInfo* stoneInfo = BlocksManager::GetBlockInfoByName("stone");
    const BlockInfo* sandInfo = BlocksManager::GetBlockInfoByName("sand");
    const BlockInfo* waterInfo = BlocksManager::GetBlockInfoByName("water");
    const BlockInfo* oakWoodInfo = BlocksManager::GetBlockInfoByName("oak_wood");
    const BlockInfo* leavesInfo = BlocksManager::GetBlockInfoByName("leaves");
    const BlockInfo* shortGrassInfo = BlocksManager::GetBlockInfoByName("short_grass");
    const BlockInfo* tulipInfo = BlocksManager::GetBlockInfoByName("tulip");
    const BlockInfo* dandelionInfo = BlocksManager::GetBlockInfoByName("dandelion");

    m_airBlock = new Block(airInfo);
    m_grassBlock = new Block(grassInfo);
    m_dirtBlock = new Block(dirtInfo);
    m_stoneBlock = new Block(stoneInfo);
    m_sandBlock = new Block(sandInfo);
    m_waterBlock = new Block(waterInfo);
    m_oakWoodBlock = new Block(oakWoodInfo);
    m_leavesBlock = new Block(leavesInfo);
    m_shortGrassBlock = new Block(shortGrassInfo);
    m_tulipBlock = new Block(tulipInfo);
    m_dandelionBlock = new Block(dandelionInfo);
}

void ChunkGenerator::GenerateTerrain(ChunkInstance& chunk, int worldX, int worldZ, 
                                     std::vector<std::vector<uint8_t>>& heightmap)
{
    if (heightmap.empty() || heightmap[0].empty()) return;

    const int mapWidth = heightmap[0].size();
    const int mapDepth = heightmap.size();
    
    for (int x = 0; x < CHUNK_LENGTH; ++x)
    {
        int globalX = worldX + x;
        if (globalX >= mapWidth) continue;
        
        for (int z = 0; z < CHUNK_WIDTH; ++z)
        {
            int globalZ = worldZ + z;
            if (globalZ >= mapDepth) continue;

            float surfaceHeightFloat = GetSurfaceHeightAt(globalX, globalZ, heightmap, mapWidth, mapDepth);
            int surfaceHeight = static_cast<int>(surfaceHeightFloat);
            
            if (surfaceHeight < 1) surfaceHeight = 1;
            if (surfaceHeight >= CHUNK_HEIGHT) surfaceHeight = CHUNK_HEIGHT - 1;
            
            float normalizedHeight = surfaceHeightFloat / 25.0f;
            
            for (int y = 0; y < surfaceHeight; ++y)
            {
                if (y < STONE_BASE_HEIGHT)
                {
                    chunk.SetBlock(x, y, z, *m_stoneBlock);
                }
                else if (y < surfaceHeight - DIRT_LAYERS)
                {
                    if (surfaceHeightFloat < 10.0f)
                    {
                        chunk.SetBlock(x, y, z, *m_sandBlock);
                    }
                    else if (surfaceHeightFloat >= 12.0f)
                    {
                        chunk.SetBlock(x, y, z, *m_stoneBlock);
                    }
                    else
                    {
                        chunk.SetBlock(x, y, z, *m_dirtBlock);
                    }
                }
                else if (y < surfaceHeight - 1)
                {
                    chunk.SetBlock(x, y, z, *m_dirtBlock);
                }
                else
                {
                    if (surfaceHeightFloat < 10.0f)
                    {
                        chunk.SetBlock(x, y, z, *m_sandBlock);
                    }
                    else
                    {
                        chunk.SetBlock(x, y, z, *m_grassBlock);
                    }
                }
            }
            
            if (surfaceHeight <= WATER_LEVEL && surfaceHeight + 1 < CHUNK_HEIGHT)
            {
             chunk.SetBlock(x, WATER_LEVEL, z, *m_waterBlock);
            }
        }
    }
}

float ChunkGenerator::GetSurfaceHeightAt(int x, int z, std::vector<std::vector<uint8_t>>& heightmap, int mapWidth, int mapDepth)
{
    if (heightmap.empty() || heightmap[0].empty() || x < 0 || z < 0 || x >= mapWidth || z >= mapDepth)
        return 1.0f;
    
    return static_cast<float>(heightmap[z][x]);
}


void ChunkGenerator::GenerateTrees(ChunkInstance& chunk, int worldX, int worldZ)
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> treeCountDist(0, MAX_TREES_PER_CHUNK);
    std::uniform_int_distribution<int> heightDist(TREE_HEIGHT_MIN, TREE_HEIGHT_MAX);
    std::uniform_int_distribution<int> posDist(2, CHUNK_LENGTH - 3);
    std::uniform_int_distribution<int> flowerTypeDist(0, 1);
    
    int treeCount = treeCountDist(m_randomEngine);
    
    for (int t = 0; t < treeCount; ++t)
    {
        if (dist(m_randomEngine) > TREE_PROBABILITY) continue;
        
        int treeX = posDist(m_randomEngine);
        int treeZ = posDist(m_randomEngine);
        int treeHeight = heightDist(m_randomEngine);
        
        int surfaceY = 0;
        for (int y = CHUNK_HEIGHT - 1; y >= 0; --y)
        {
            Block* block = chunk.GetBlock(treeX, y, treeZ);
            if (block && block->blockInfo->block_name == "grass_block")
            {
                surfaceY = y + 1;
                break;
            }
        }
        
        if (surfaceY == 0 || surfaceY + treeHeight >= CHUNK_HEIGHT - 2) continue;
        
        for (int y = surfaceY; y < surfaceY + treeHeight; ++y)
        {
            chunk.SetBlock(treeX, y, treeZ, *m_oakWoodBlock);
        }
        
        int canopyY = surfaceY + treeHeight - 2;
        for (int dx = -TREE_CANOPY_RADIUS; dx <= TREE_CANOPY_RADIUS; ++dx)
        {
            for (int dz = -TREE_CANOPY_RADIUS; dz <= TREE_CANOPY_RADIUS; ++dz)
            {
                for (int dy = 0; dy <= 2; ++dy)
                {
                    int cx = treeX + dx;
                    int cy = canopyY + dy;
                    int cz = treeZ + dz;
                    
                    if (cx < 0 || cx >= CHUNK_LENGTH || cz < 0 || cz >= CHUNK_WIDTH || cy < 0 || cy >= CHUNK_HEIGHT)
                        continue;
                    
                    if (dx == 0 && dz == 0 && dy < 2) continue;
                    
                    int distSq = dx * dx + dz * dz + dy * dy;
                    if (distSq <= TREE_CANOPY_RADIUS * TREE_CANOPY_RADIUS)
                    {
                        chunk.SetBlock(cx, cy, cz, *m_leavesBlock);
                    }
                }
            }
        }
        
        for (int y = canopyY + 3; y < surfaceY + treeHeight + 1 && y < CHUNK_HEIGHT; ++y)
        {
            chunk.SetBlock(treeX, y, treeZ, *m_leavesBlock);
        }
    }
    
    for (int x = 0; x < CHUNK_LENGTH; ++x)
    {
        for (int z = 0; z < CHUNK_WIDTH; ++z)
        {
            for (int y = 1; y < CHUNK_HEIGHT; ++y)
            {
                Block* block = chunk.GetBlock(x, y, z);
                Block* blockBelow = chunk.GetBlock(x, y - 1, z);
                
                if (block && blockBelow && block->blockInfo->block_name == "air" && 
                    blockBelow->blockInfo->block_name == "grass_block")
                {
                    float randVal = dist(m_randomEngine);
                    
                    if (randVal < GRASS_PROBABILITY)
                    {
                        chunk.SetBlock(x, y, z, *m_shortGrassBlock);
                    }
                    else if (randVal < GRASS_PROBABILITY + FLOWER_PROBABILITY)
                    {
                        if (flowerTypeDist(m_randomEngine) == 0)
                        {
                            chunk.SetBlock(x, y, z, *m_tulipBlock);
                        }
                        else
                        {
                            chunk.SetBlock(x, y, z, *m_dandelionBlock);
                        }
                    }
                }
            }
        }
    }
}

void ChunkGenerator::GenerateChunk(ChunkInstance& chunk, std::vector<std::vector<uint8_t>>& data)
{
    const int worldX = chunk.GetChunkX() * CHUNK_LENGTH;
    const int worldZ = chunk.GetChunkZ() * CHUNK_WIDTH;
    
    for (int x = 0; x < CHUNK_LENGTH; ++x)
    {
        for (int y = 0; y < CHUNK_HEIGHT; ++y)
        {
            for (int z = 0; z < CHUNK_WIDTH; ++z)
            {
                chunk.SetBlock(x, y, z, *m_airBlock);
            }
        }
    }
    
   GenerateTerrain(chunk, worldX, worldZ, data);
   GenerateTrees(chunk, worldX, worldZ);
}