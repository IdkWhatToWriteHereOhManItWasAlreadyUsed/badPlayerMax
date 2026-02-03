#include <ThreadPool/ThreadPool.h>
#include "GlobalThreadPool.h"
#include "chunk/rendering/RayCaster.h"
#include "VideoWorld.h"
#include "chunk/generation/ChunkGenerator.h"
#include "block/Block.h"
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>
#include <GL/glew.h>
#include <glm/fwd.hpp>
#include <renderer/GraphicalDataBuffers/BlocksGraphicalDataBuffer.h>
#include "chunk/ChunkInstance.h"
#include "chunk/rendering/ChunkGraphicalData.h"
#include "videoWorld/chunk/model/Chunk.h"
#include <array>

VideoWorld::VideoWorld()
{
}

void VideoWorld::Initialise(int width, int height)
{    
    m_width = width / CHUNK_WIDTH;
    m_height = height / CHUNK_LENGTH;

    m_loadedChunks.resize(m_width);
    for (auto& chunkLine: m_loadedChunks)
    {
        chunkLine.resize(m_height);
        for (auto& chunk: chunkLine)
        {
            chunk = std::make_unique<ChunkInstance>(0,0);
        }
    }

    m_numThreads = globalThreadPool.GetThreadCount();
    m_idsToDelete.resize(m_numThreads);
   // m_blocksGraphicalData.resize(m_numThreads);
}

VideoWorld::VideoWorld(int seed)
{
}

void VideoWorld::SetSeed(int seed)
{
}

VideoWorld::~VideoWorld()
{
}

void VideoWorld::Update(float playerX, float playerZ)
{
    //UpdateChunks(playerX, playerZ);
}

const Block* VideoWorld::GetBlock(int x, int y, int z) const
{
   return nullptr;
}


void VideoWorld::SetBlock(int x, int y, int z, Block block)
{
    return;
}

RayCastResult VideoWorld::GetBlockPlayerLooksAt(glm::vec3 rayStart, glm::vec3 rayDir, GLfloat distance) const
{
    return RayCaster::castRay(rayStart, rayDir, this, distance);
}

std::array<BlocksGraphicalDataBuffer, static_cast<std::size_t>(GeometryType::Count)>& VideoWorld::GetBlocksGraphicalData()
{
    return m_blocksGraphicalData;
}


void VideoWorld::DisplayFrame(std::vector<std::vector<uint8_t>>& data)
{
    m_numThreads = globalThreadPool.GetThreadCount();

    int rowsPerStrip = m_width / m_numThreads;
    int extraRows = m_width % m_numThreads;

    int currentX = 0;
    for (int stripIndex = 0; stripIndex < m_numThreads; ++stripIndex) 
    {
        int stripHeight = rowsPerStrip;
        if (stripIndex == m_numThreads - 1)
            stripHeight += extraRows;
        if (stripHeight == 0) continue;
        
        int endX = currentX + stripHeight;
        
        auto task = std::make_shared<Task<void, VideoWorld*, 
                                         std::vector<std::vector<uint8_t>>*,
                                         int, int, int>>
        (
            [](VideoWorld* world, 
               std::vector<std::vector<uint8_t>>* dataPtr, 
               int start, int end, int threadIndex) 
            {
                world->GenerateStrip(*dataPtr, start, end, threadIndex);
            },
            this, &data, currentX, endX, stripIndex
        );
        
        m_threadPool->AddTask(task);
        currentX = endX;
    }
    
    m_threadPool->WaitAll();
    currentX = 0;

    for (int stripIndex = 0; stripIndex < m_numThreads; ++stripIndex) 
    {
        int stripHeight = rowsPerStrip;
        if (stripIndex == m_numThreads - 1)
            stripHeight += extraRows;
        if (stripHeight == 0) continue;
        
        int endX = currentX + stripHeight;
        
        auto task = std::make_shared<Task<void, VideoWorld*, std::vector<std::vector<uint8_t>>*,
                                         int, int, int>>
        (
            [](VideoWorld* world, 
               std::vector<std::vector<uint8_t>>* dataPtr, 
               int start, int end, int threadIndex) 
            {
                world->GenerateMeshesOfStrip(*dataPtr, start, end, threadIndex);
            },
            this, &data, currentX, endX, stripIndex
        );
        
        m_threadPool->AddTask(task);
        currentX = endX;
    }
    
    m_threadPool->WaitAll();
    currentX = 0;

    /*
    for (int threadIndex = 0; threadIndex < m_numThreads; ++threadIndex)    
    {       
       for (auto gtype = 0; gtype < static_cast<int>(GeometryType::Count); gtype++)
        {
            for (auto id: m_idsToDelete[threadIndex][gtype])
                m_blocksGraphicalData[threadIndex][gtype].DeleteAt(id);
        
            //m_idsToDelete[threadIndex][gtype].clear();
        }         
    }*/


    /*
        на удивление эта хрень без разделения на потоки пашет лучше, чем с разделением
        видимо я недооценил стоимость добавления тасок в тредпул
        ну это мб даже и логично было
        Боттлнек, связанный с сранием из кучи потоков в один буфер подтвердился
        на удивление, в одном потоке с его учетом фпс тоже выше получаеися
        а хотя это же логично, у меня в буферах ресайз маленький
        а может и нет
        ну, работает хорошо и так сойдет
    */
    for (int stripIndex = 0; stripIndex < m_numThreads; ++stripIndex) 
    {
        int stripHeight = rowsPerStrip;
        if (stripIndex == m_numThreads - 1)
            stripHeight += extraRows;
        if (stripHeight == 0) continue;
        
        int endX = currentX + stripHeight;

        for (int i = currentX; i < endX; i++)
        {                  
            for (auto j = 0; j < m_loadedChunks[i].size(); j++)
            {
                for (auto gtype= 0; gtype < static_cast<int>(GeometryType::Count); gtype++)
                { 
                    auto& checkedChunk = m_loadedChunks[i][j];
                    auto& chunkGraphicalData = checkedChunk->GetGraphicalData();

                    std::lock_guard lock(m_worldMutex);
                    m_blocksGraphicalData[gtype].DeleteAt
                    (
                        checkedChunk->GetMeshID(gtype)
                    );
                    checkedChunk->SetMeshID
                    (gtype, 
                        m_blocksGraphicalData[gtype].Write
                        (chunkGraphicalData.allBlockTypesMeshData[gtype].Data, chunkGraphicalData.aabb)
                    );
                }
            }
        }

        currentX = endX;
    }
    return;



    //
    for (int stripIndex = 0; stripIndex < m_numThreads; ++stripIndex) 
    {
        int stripHeight = rowsPerStrip;
        if (stripIndex == m_numThreads - 1)
            stripHeight += extraRows;
        if (stripHeight == 0) continue;
        
        int endX = currentX + stripHeight;
        
        auto task = std::make_shared<Task<void, VideoWorld*, 
                                         int, int, int>>(
            [this](VideoWorld* world, 
               int start, int end, int threadIndex) 
            {
                for (auto gtype= 0; gtype < static_cast<int>(GeometryType::Count); gtype++)
                {      
                    /* 
                    for (auto id: m_idsToDelete[threadIndex][gtype])
                        m_blocksGraphicalData[threadIndex][gtype].DeleteAt(id);
                    
                    m_idsToDelete[threadIndex][gtype].clear();*/
                    for (int i = start; i < end; i++)
                    {
                        for (auto j = 0; j < m_loadedChunks[i].size(); j++)
                        { 
                            auto& checkedChunk = m_loadedChunks[i][j];
                            auto& chunkGraphicalData = checkedChunk->GetGraphicalData();

                            std::lock_guard lock(m_worldMutex);
                            m_blocksGraphicalData[gtype].DeleteAt
                            (
                                checkedChunk->GetMeshID(gtype)
                            );
                            checkedChunk->SetMeshID
                            (gtype, 
                                m_blocksGraphicalData[gtype].Write
                                (chunkGraphicalData.allBlockTypesMeshData[gtype].Data, chunkGraphicalData.aabb)
                            );
                        }
                    }
                }
            },
            this, currentX, endX, 0
        );

        /*
        for (auto gtype = 0; gtype < static_cast<int>(GeometryType::Count); gtype++)
        {
            auto threadIndex = stripIndex;
            for (auto id: m_idsToDelete[threadIndex][gtype])
                m_blocksGraphicalData[threadIndex][gtype].DeleteAt(id);
        
            m_idsToDelete[threadIndex][gtype].clear();
        }   */
        
        m_threadPool->AddTask(task);
        currentX = endX;
    }

    m_threadPool->WaitAll();

    /*
    for (auto i = 0; i < static_cast<int>(GeometryType::Count); i++)
    {
        for (auto& chunkLine: m_loadedChunks)
            for (auto& checkedChunk: chunkLine)           
            {
                auto& chunkGraphicalData = checkedChunk->GetGraphicalData();
                checkedChunk->SetMeshID(i, m_blocksGraphicalData[i].Write(chunkGraphicalData.allBlockTypesMeshData[i].Data, chunkGraphicalData.aabb));
            }
    }*/
}


void VideoWorld::GenerateStrip(std::vector<std::vector<uint8_t>>& data, int startX, int endX, int threadIndex)
{
    for (int i = startX; i < endX; i++)
    {
        for (auto j = 0; j < m_loadedChunks[i].size(); j++)
        {
            auto newChunk = std::make_unique<ChunkInstance>(i, j);
            ChunkGenerator::GenerateChunk(*newChunk, data);
            newChunk->InvalidateMeshData(); 

            std::array<GLuint, static_cast<int>(GeometryType::Count)> idsToDelete;

            for (auto gtype = 0; gtype < static_cast<int>(GeometryType::Count); gtype++)
            {
                idsToDelete[gtype] = m_loadedChunks[i][j]->GetMeshID(gtype);
            }
            
            for (auto gtype = 0; gtype < static_cast<int>(GeometryType::Count); gtype++)
            {
                newChunk->SetMeshID(gtype, idsToDelete[gtype]);
            }   
         
            m_loadedChunks[i][j].reset();
            m_loadedChunks[i][j] = std::move(newChunk);      
        }
    }
}

void VideoWorld::GenerateMeshesOfStrip(std::vector<std::vector<uint8_t>>& data, int startX, int endX, int threadIndex)
{
    for (int i = startX; i < endX; i++)
    {
        for (auto j = 0; j < m_loadedChunks[i].size(); j++)
        { 
            auto x = i;
            auto z = j;

            ChunkInstance* left = nullptr;
            ChunkInstance* right = nullptr;
            ChunkInstance* front = nullptr;
            ChunkInstance* back = nullptr;

            if (x - 1 >= 0)
                left = m_loadedChunks[x - 1][z].get();

            if (z - 1 >= 0)
                back = m_loadedChunks[x][z - 1].get();

            if (x + 1 < m_loadedChunks.size())
                right = m_loadedChunks[x + 1][z].get();

            if (z + 1 < m_loadedChunks[x].size())
                front = m_loadedChunks[x][z + 1].get();

            std::array<GLuint, static_cast<int>(GeometryType::Count)> idsToDelete;
            for (auto gtype = 0; gtype < static_cast<int>(GeometryType::Count); gtype++)
            {
                idsToDelete[gtype] = m_loadedChunks[i][j]->GetMeshID(gtype);
            }

            m_loadedChunks[x][z]->GenerateGraphicalDataIfNeeded(left, right, front, back);
                   
            for (auto gtype = 0; gtype < static_cast<int>(GeometryType::Count); gtype++)
            {
                m_loadedChunks[x][z]->SetMeshID(gtype, idsToDelete[gtype]);
            }   
        }
    }
}
