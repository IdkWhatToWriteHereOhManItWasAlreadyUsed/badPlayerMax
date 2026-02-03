#pragma once

#include <cstdint>
#include <memory>
#include <renderer/GraphicalDataBuffers/BlocksGraphicalDataBuffer.h>
#include <ThreadPool/ThreadPool.h>
#include "chunk/ChunkInstance.h"
#include <cmath>
#include <GlobalThreadPool.h>
#include <utility>
#include <GL/glew.h>
#include <glm/fwd.hpp>
#include "block/Block.h"
#include <array>
#include "chunk/rendering/ChunkGraphicalData.h"
#include <mutex>
#include <libs/robin_hood.h>
#include <vector>

struct RayCastResult;
class RayCaster;

struct PairHash 
{
    size_t operator()(const std::pair<int, int>& p) const noexcept 
    {
        return robin_hood::hash_int(p.first) ^ (robin_hood::hash_int(p.second) << 1);
    }
};

inline int WorldToChunkCoord(float coord) {
    return static_cast<int>(std::floor(coord / 16.0f));
}

inline std::pair<int, int> WorldToChunkCoords(float worldX, float worldZ) {
    return {
        static_cast<int>(std::floor(worldX / 16.0f)),
        static_cast<int>(std::floor(worldZ / 16.0f))
    };
}

class VideoWorld 
{
private:
    //robin_hood::unordered_flat_map<std::pair<int, int>, std::unique_ptr<ChunkInstance>, PairHash> m_loadedChunks;

    std::array<BlocksGraphicalDataBuffer, static_cast<std::size_t>(GeometryType::Count)> m_blocksGraphicalData;
    std::mutex m_worldMutex;
    int m_loadingDistance = 6;
    int m_height, m_width;
    int m_numThreads;

    std::vector<std::vector<std::unique_ptr<ChunkInstance>>>  m_loadedChunks;
    ThreadPool* m_threadPool = &globalThreadPool;
public:
    VideoWorld();
    VideoWorld(int seed);
    ~VideoWorld();

    void Initialise(int videoWidth, int videoHeight);
    void SetLoadingDistance(int distance) { m_loadingDistance = distance; }
    int GetLoadingDistance() const { return m_loadingDistance; }
    void SetSeed(int seed);
    void Update(float playerX, float playerZ);

    const Block* GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, Block block);
    RayCastResult GetBlockPlayerLooksAt(glm::vec3 rayStart, glm::vec3 rayDir, GLfloat distance = 16.0f) const;

    std::array<BlocksGraphicalDataBuffer, static_cast<std::size_t>(GeometryType::Count)>& GetBlocksGraphicalData();
    int GetBuffersCount() { return m_blocksGraphicalData.size();}

    std::vector<std::array<robin_hood::unordered_set<GLuint>, static_cast<std::size_t>(GeometryType::Count)>> m_idsToDelete;
    void DisplayFrame(std::vector<std::vector<uint8_t>>& data);
private:
    //void UpdateChunks(float playerX, float playerZ);
    void GenerateStrip(std::vector<std::vector<uint8_t>>& data, int startX, int endX,int threadIndex);
    void GenerateMeshesOfStrip(std::vector<std::vector<uint8_t>>& data, int startX, int endX, int threadIndex);
};
