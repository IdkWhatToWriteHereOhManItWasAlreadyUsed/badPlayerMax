#pragma once

#include <videoWorld/chunk/ChunkInstance.h>
#include <unordered_map>
#include <videoWorld/block/BlocksManager.h>
#include <random>

class ChunkGenerator
{
private:
    static std::unordered_map<std::string, const BlockInfo*> m_blockCache;
    static std::mt19937 m_randomEngine;

public:
    static void InitializeBlocks();
    static void GenerateChunk(ChunkInstance& chunk, std::vector<std::vector<uint8_t>>& data);

private:
    static void GenerateTerrain(ChunkInstance& chunk, int worldX, int worldZ, std::vector<std::vector<uint8_t>>& heightmap);
    static void GenerateTrees(ChunkInstance& chunk, int worldX, int worldZ);
    static void GenerateWater(ChunkInstance& chunk, int waterLevel);
    static float GetSurfaceHeightAt(int x, int z, std::vector<std::vector<uint8_t>>& heightmap, int mapWidth, int mapDepth);
    
    static const BlockInfo* GetBlockInfo(const std::string& name);
    
private:
    static inline Block* m_shortGrassBlock = nullptr;
    static inline Block* m_tulipBlock = nullptr;
    static inline Block* m_dandelionBlock = nullptr;
    
    static constexpr float GRASS_PROBABILITY = 0.05f;
    static constexpr float FLOWER_PROBABILITY = 0.03f;
    static inline Block* m_airBlock = nullptr;
    static inline Block* m_grassBlock = nullptr;
    static inline Block* m_dirtBlock = nullptr;
    static inline Block* m_stoneBlock = nullptr;
    static inline Block* m_sandBlock = nullptr;
    static inline Block* m_waterBlock = nullptr;
    static inline Block* m_oakWoodBlock = nullptr;
    static inline Block* m_leavesBlock = nullptr;
    
    static constexpr int WATER_LEVEL = 8;
    static constexpr int STONE_BASE_HEIGHT = 4;
    static constexpr int DIRT_LAYERS = 3;
    static constexpr float TREE_PROBABILITY = 0.8f;
    static constexpr int MAX_TREES_PER_CHUNK = 3;
    static constexpr int TREE_HEIGHT_MIN = 4;
    static constexpr int TREE_HEIGHT_MAX = 8;
    static constexpr int TREE_CANOPY_RADIUS = 4;
};